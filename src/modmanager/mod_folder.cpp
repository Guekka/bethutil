/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_folder.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/common/filesystem.hpp"

#include <binary_io/memory_stream.hpp>
#include <flux.hpp>

#include <atomic>
#include <condition_variable>
#include <exception>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace btu::modmanager {

// Lets worker tasks enqueue child work without waiting on the same pool.
class ModFolderTaskGroup
{
public:
    struct Task
    {
        std::shared_ptr<const Path> path;
        std::future<void> future;
    };

private:
    class Completion
    {
        ModFolderTaskGroup &group_;

    public:
        explicit Completion(ModFolderTaskGroup &group)
            : group_(group)
        {
        }

        ~Completion() { group_.task_finished(); }
    };

    common::ThreadPool &thread_pool_;
    std::mutex mutex_;
    std::condition_variable completed_;
    std::size_t pending_tasks_ = 0;
    std::list<Task> tasks_;

    void task_finished()
    {
        auto lock = std::lock_guard{mutex_};
        if (--pending_tasks_ == 0)
            completed_.notify_one();
    }

public:
    explicit ModFolderTaskGroup(common::ThreadPool &thread_pool)
        : thread_pool_(thread_pool)
    {
    }

    template<typename Function>
    void submit(std::shared_ptr<const Path> path, Function &&function)
    {
        auto task = std::list<Task>::iterator{};
        {
            auto lock = std::unique_lock{mutex_};
            task = tasks_.insert(tasks_.end(), Task{std::move(path), {}});
            ++pending_tasks_;
        }

        auto future = std::future<void>{};
        try
        {
            future = thread_pool_.submit_task([this, task = std::forward<Function>(function)]() mutable {
                auto completion = Completion{*this};
                task();
            });
        }
        catch (...)
        {
            auto lock = std::lock_guard{mutex_};
            tasks_.erase(task);
            if (--pending_tasks_ == 0)
                completed_.notify_one();
            throw;
        }

        task->future = std::move(future);
    }

    [[nodiscard]] auto wait() -> std::list<Task>
    {
        auto lock = std::unique_lock{mutex_};
        completed_.wait(lock, [this] { return pending_tasks_ == 0; });
        return std::move(tasks_);
    }
};

ModFolder::ModFolder(Path directory, bsa::Settings bsa_settings, bool ignore_existing_archives)
    : ModFolder(std::move(directory), std::move(bsa_settings), ignore_existing_archives, 0)
{
}

ModFolder::ModFolder(Path directory,
                     bsa::Settings bsa_settings,
                     bool ignore_existing_archives,
                     const std::uint32_t worker_count)
    : dir_(std::move(directory))
    , bsa_settings_(BTU_MOV(bsa_settings))
    , ignore_existing_archives_(ignore_existing_archives)
    , thread_pool_(common::make_thread_pool_with_count(worker_count))
{
}

auto ModFolder::size() noexcept -> size_t
{
    class SizeIterator final : public ModFolderIterator
    {
    public:
        std::atomic_size_t size{};

        [[nodiscard]] auto archive_too_large(const Path & /*archive_path*/,
                                             ArchiveTooLargeState /*state*/) noexcept
            -> ArchiveTooLargeAction override
        {
            return Skip;
        }

        void process_file(ModFile /*file*/) noexcept override { size.fetch_add(1, std::memory_order_relaxed); }
    };

    auto iterator = SizeIterator{};
    iterate(iterator);
    return iterator.size.load(std::memory_order_relaxed);
}

void ModFolder::iterate(ModFolderIterator &iterator) noexcept
{
    // creates a ModFolderTransformer based on the user-provided ModFolderIterator
    class ModFolderProcessorReadOnly final : public ModFolderTransformer
    {
    public:
        explicit ModFolderProcessorReadOnly(ModFolderIterator &iterator) noexcept
            : iterator_(iterator)
        {
        }

        [[nodiscard]] auto archive_too_large(const Path &archive_path, ArchiveTooLargeState state) noexcept
            -> ArchiveTooLargeAction override
        {
            return iterator_.get().archive_too_large(archive_path, state);
        }

        [[nodiscard]] auto transform_file(ModFile file) noexcept
            -> std::optional<std::vector<std::byte>> override
        {
            iterator_.get().process_file(std::move(file));
            return std::nullopt;
        }

        [[nodiscard]] auto stop_requested() const noexcept -> bool override
        {
            return iterator_.get().stop_requested();
        }

    private:
        std::reference_wrapper<ModFolderIterator> iterator_;
    } transformer(iterator);

    transform(transformer);
}

/**
 * \brief Guesses the target archive version of the given archive based on the provided settings.
 *
 * \param archive The archive to guess the target version for.
 * \param bsa_settings The settings to be used for guessing the version.
 *
 * \return An optional `bsa::ArchiveVersion` indicating the guessed target version
 *         or `std::nullopt` if the target version could not be determined. This can also happen if the
 *         target version is the same as the current version.
 *
 * \see bsa::Archive
 * \see bsa::Settings
 */
[[nodiscard]] auto guess_target_archive_version(const bsa::Archive &archive,
                                                const bsa::Settings &bsa_settings) noexcept
    -> std::optional<bsa::ArchiveVersion>
{
    const auto target = bsa_settings.version;

    if (target == archive.version())
        return std::nullopt;

    return target;
}

void transform_loose_file(const Path &absolute_path,
                          const Path &dir,
                          ModFolderTransformer &transformer) noexcept
{
    if (transformer.stop_requested())
        return;

    const auto relative_path = absolute_path.lexically_relative(dir);

    const auto file_data = common::Lazy<tl::expected<std::vector<std::byte>, common::Error>>(
        [&absolute_path] { return common::read_file(absolute_path); });

    if (const auto transformed = transformer.transform_file(ModFile{relative_path, file_data}))
    {
        if (!common::write_file(absolute_path, *transformed))
            transformer.failed_to_write_transformed_file(relative_path, *transformed);
    }
}

[[nodiscard]] auto want_to_skip_archive(const Path &archive_path,
                                        ModFolderTransformer &transformer,
                                        ModFolderIteratorBase::ArchiveTooLargeState state) noexcept -> bool
{
    return transformer.archive_too_large(archive_path, state)
           == ModFolderIteratorBase::ArchiveTooLargeAction::Skip;
}

void transform_archive_file_inner(ModFolderTransformer &transformer,
                                  std::atomic_bool &any_file_changed,
                                  bsa::Archive::value_type &pair)
{
    if (transformer.stop_requested())
        return;

    auto &[relative_path, file] = pair;

    auto file_data = common::Lazy<tl::expected<std::vector<std::byte>, common::Error>>(
        [&pair]() -> tl::expected<std::vector<std::byte>, common::Error> {
            auto buffer = binary_io::any_ostream{binary_io::memory_ostream{}};
            if (!pair.second.write(buffer))
                // TODO: better error here?
                return tl::make_unexpected(common::Error(std::error_code(errno, std::system_category())));

            return buffer.get<binary_io::memory_ostream>().rdbuf();
        });

    auto transformed = transformer.transform_file(ModFile{relative_path, file_data});
    if (transformed)
    {
        const bool res = file.read(*transformed);
        if (!res)
            transformer.failed_to_read_transformed_file(relative_path, *transformed);

        if (res)
            any_file_changed.store(true);
    }
}

[[nodiscard]] auto change_archive_version_if_needed(
    bsa::Archive &archive, const bsa::Settings &bsa_settings) noexcept -> tl::expected<bool, common::Error>
{
    if (const auto target_version = guess_target_archive_version(archive, bsa_settings))
    {
        const auto res = archive.set_version(*target_version);
        return res.map([] { return true; });
    }
    return false;
}

class ArchiveTransformState : public std::enable_shared_from_this<ArchiveTransformState>
{
    std::shared_ptr<const Path> archive_path_;
    ModFolderTransformer &transformer_;
    const bsa::Settings &bsa_settings_;
    bsa::Archive archive_;
    std::atomic_size_t pending_members_{1};
    std::atomic_bool any_file_changed_{false};
    std::atomic_bool cancelled_{false};
    std::mutex error_mutex_;
    std::exception_ptr error_;

    void record_error(std::exception_ptr error)
    {
        auto lock = std::lock_guard{error_mutex_};
        if (!error_)
            error_ = std::move(error);
    }

    void process_member(bsa::Archive::value_type &pair)
    {
        if (transformer_.stop_requested())
        {
            cancelled_.store(true);
        }
        else
        {
            try
            {
                transform_archive_file_inner(transformer_, any_file_changed_, pair);
            }
            catch (...)
            {
                record_error(std::current_exception());
            }
        }

        complete_member();
    }

    void complete_member()
    {
        if (pending_members_.fetch_sub(1) == 1)
            finish();
    }

    void finish()
    {
        auto error = std::exception_ptr{};
        {
            auto lock = std::lock_guard{error_mutex_};
            error     = error_;
        }
        if (error)
            std::rethrow_exception(error);

        if (cancelled_.load() || transformer_.stop_requested())
            return;

        const auto &archive_path    = *archive_path_;
        const auto version_changed = change_archive_version_if_needed(archive_, bsa_settings_);
        if (!version_changed)
        {
            transformer_.failed_to_change_archive_version(archive_path, version_changed.error());
            return;
        }

        if (const auto arch_size = archive_.file_size(); arch_size > bsa_settings_.max_size)
        {
            if (want_to_skip_archive(archive_path,
                                     transformer_,
                                     ModFolderIteratorBase::ArchiveTooLargeState::AfterProcessing))
                return;
        }

        if (transformer_.stop_requested())
            return;

        if (any_file_changed_.load() || version_changed.value_or(false))
        {
            auto path = archive_path;
            if (path.extension() != bsa_settings_.extension)
                path.replace_extension(bsa_settings_.extension);

            if (!std::move(archive_).write(path))
            {
                transformer_.failed_to_write_archive(archive_path, path);
                return;
            }

            if (!equivalent(archive_path, path))
                fs::remove(archive_path);
        }
    }

public:
    ArchiveTransformState(std::shared_ptr<const Path> archive_path,
                          ModFolderTransformer &transformer,
                          const bsa::Settings &bsa_settings,
                          bsa::Archive archive)
        : archive_path_(std::move(archive_path))
        , transformer_(transformer)
        , bsa_settings_(bsa_settings)
        , archive_(std::move(archive))
    {
    }

    void schedule_members(ModFolderTaskGroup &tasks)
    {
        const auto self = shared_from_this();
        for (auto &pair : archive_)
        {
            if (transformer_.stop_requested())
            {
                cancelled_.store(true);
                break;
            }

            pending_members_.fetch_add(1);
            try
            {
                auto *member = &pair;
                tasks.submit(archive_path_, [self, member] { self->process_member(*member); });
            }
            catch (...)
            {
                record_error(std::current_exception());
                cancelled_.store(true);
                pending_members_.fetch_sub(1);
                break;
            }
        }

        // Keep a sentinel while scheduling so fast workers cannot finalize a partially queued archive.
        complete_member();
    }
};

void transform_archive_file(const Path &archive_path,
                            ModFolderTransformer &transformer,
                            const bsa::Settings &bsa_settings,
                            ModFolderTaskGroup &tasks,
                            std::shared_ptr<const Path> task_path)
{
    if (const auto arch_size = file_size(archive_path); arch_size > bsa_settings.max_size)
    {
        if (want_to_skip_archive(archive_path,
                                 transformer,
                                 ModFolderIteratorBase::ArchiveTooLargeState::BeforeProcessing))
            return;
    }

    auto opt_arch = bsa::Archive::read(archive_path);
    if (!opt_arch)
    {
        transformer.failed_to_read_archive(archive_path);
        return;
    }

    auto state =
        std::make_shared<ArchiveTransformState>(std::move(task_path), transformer, bsa_settings, std::move(*opt_arch));
    state->schedule_members(tasks);
}

void ModFolder::transform(ModFolderTransformer &transformer) noexcept
{
    auto is_arch = [](const Path &file_name) {
        const auto ext = common::to_lower(file_name.extension().u8string());
        return common::contains(bsa::k_archive_extensions, ext);
    };

    auto files = flux::from_range(fs::recursive_directory_iterator(dir_))
                     .filter([](auto &&e) { return e.is_regular_file(); })
                     .map([](auto &&e) { return e.path(); })
                     .to<std::vector>();

    auto tasks = ModFolderTaskGroup{thread_pool_};
    for (const auto &file_path : files)
    {
        if (transformer.stop_requested())
            break;

        if (is_arch(file_path) && ignore_existing_archives_)
            continue;

        auto task_path = std::make_shared<const Path>(file_path);
        tasks.submit(task_path, [this, file_path, task_path, &is_arch, &transformer, &tasks] {
            if (is_arch(file_path)) [[unlikely]]
                transform_archive_file(file_path, transformer, bsa_settings_, tasks, task_path);
            else [[likely]]
                transform_loose_file(file_path, dir_, transformer);
        });
    }

    for (auto &task : tasks.wait())
        task.future.wait();
};
} // namespace btu::modmanager
