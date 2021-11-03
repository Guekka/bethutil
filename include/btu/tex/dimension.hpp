#pragma once

#include "btu/tex/detail/common.hpp"
#include "btu/tex/detail/formats.hpp"

namespace btu::tex {

struct Dimension
{
    size_t w;
    size_t h;
    auto operator<=>(const Dimension &) const noexcept = default;
};

namespace util {
struct ResizeRatio
{
    uint8_t ratio;
    Dimension min;
};

[[nodiscard]] constexpr auto is_pow2(size_t num) noexcept -> bool
{
    return (!!num & !(num & (num - 1))) != 0; // NOLINT
}

[[nodiscard]] constexpr auto upper_pow2(size_t num) noexcept -> size_t
{
    size_t res = 1;
    while (res < num)
        res *= 2;

    return res;
}

[[nodiscard]] constexpr auto nearest_pow2(size_t num) noexcept -> size_t
{
    const auto upper = upper_pow2(num);
    const auto lower = upper / 2;
    if (upper - num > num - lower)
        return lower;
    return upper;
}

[[nodiscard]] constexpr auto sanitize_dimensions(Dimension dim,
                                                 uint8_t ratio_threshold = 3,
                                                 uint8_t pow2_threshold  = 3) noexcept -> Dimension
{
    // Prevent 0s
    dim.w = std::max<size_t>(dim.w, 1);
    dim.h = std::max<size_t>(dim.h, 1);

    auto abs            = [](auto x) { return x > 0 ? x : -x; };
    auto compare_thresh = [abs](auto num, auto div, auto thresh) {
        return abs(1 - static_cast<double>(num) / div) < (static_cast<double>(thresh) / 100);
    };

    // Already a power of two
    if (is_pow2(dim.w) && is_pow2(dim.h))
        return {dim.w, dim.h};

    // Within threshold of perfect ratio
    // We set a perfect ratio, and upgrade to closest power of 2
    if (compare_thresh(dim.w, dim.h, ratio_threshold))
        dim.w = dim.h = nearest_pow2((dim.w + dim.h) / 2);

    // Each component is within threshold of pow2
    const auto near_x = nearest_pow2(dim.w);
    if (compare_thresh(dim.w, near_x, pow2_threshold))
        dim.w = near_x;

    const auto near_y = nearest_pow2(dim.h);
    if (compare_thresh(dim.h, near_y, pow2_threshold))
        dim.h = near_y;

    return dim;
}

// Makes number equal to target, and scale number2 accordingly
constexpr auto scale_fit(size_t &number, size_t target, size_t &number2) -> void
{
    const auto ratio = target / static_cast<double>(number);
    number2 *= ratio;
    number = target;
}

[[nodiscard]] constexpr auto compute_resize_dimension(Dimension dim, Dimension target) noexcept -> Dimension
{
    dim = sanitize_dimensions(dim); // We start from a sanitized base

    // Only downsize
    if (target > dim)
        return dim;

    // Good case. We already got a power of 2. We wanna preserve it, so we only divide by 2
    if (is_pow2(dim.w) || is_pow2(dim.h))
    {
        target.w *= 2;
        target.h *= 2; // Otherwise last calculation would end below target
        while (dim.w >= target.w && dim.h >= target.h)
        {
            dim.w /= 2;
            dim.h /= 2;
        }
        return dim;
    }
    // Bad case. No power of 2. Which means we can afford less precise calculations

    // If target and current have same ratio
    const auto cur_ratio    = (dim.h * 1000) / dim.w; // NOLINT
    const auto target_ratio = (target.h * 1000) / target.w;
    if (cur_ratio == target_ratio)
        return target; // We can directly return target

    // Here we know that target and source have different ratios
    const auto max_ratio = std::max(target.w / static_cast<double>(dim.w),
                                    target.h / static_cast<double>(dim.h));
    // We return the closest multiple
    return sanitize_dimensions(
        {static_cast<size_t>(dim.w * max_ratio), static_cast<size_t>(dim.h * max_ratio)});
}

[[nodiscard]] constexpr auto compute_resize_dimension(Dimension dim, ResizeRatio args) noexcept -> Dimension
{
    const auto ratio = static_cast<double>(args.ratio);
    auto target      = sanitize_dimensions(
        {static_cast<size_t>(std::round(dim.w / ratio)), static_cast<size_t>(std::round(dim.h / ratio))});

    if (target.w < args.min.w)
        scale_fit(target.w, args.min.w, target.h);
    if (target.h < args.min.h)
        scale_fit(target.h, args.min.h, target.w);

    //  Maybe those calculations changed ratio by a tiny amount, allowing to make the tex a square
    target = sanitize_dimensions(target);

    return compute_resize_dimension(dim, target);
}
} // namespace util
} // namespace btu::tex
