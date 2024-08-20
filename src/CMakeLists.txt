set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(INCLUDE_DIR "${ROOT_DIR}/include")
set(HEADER_FILES
    "${INCLUDE_DIR}/btu/common/algorithms.hpp"
    "${INCLUDE_DIR}/btu/common/error.hpp"
    "${INCLUDE_DIR}/btu/common/filesystem.hpp"
    "${INCLUDE_DIR}/btu/common/functional.hpp"
    "${INCLUDE_DIR}/btu/common/games.hpp"
    "${INCLUDE_DIR}/btu/common/metaprogramming.hpp"
    "${INCLUDE_DIR}/btu/common/path.hpp"
    "${INCLUDE_DIR}/btu/common/string.hpp"
    "${INCLUDE_DIR}/btu/common/threading.hpp"
    "${INCLUDE_DIR}/btu/bsa/pack.hpp"
    "${INCLUDE_DIR}/btu/bsa/unpack.hpp"
    "${INCLUDE_DIR}/btu/bsa/archive.hpp"
    "${INCLUDE_DIR}/btu/bsa/settings.hpp"
    "${INCLUDE_DIR}/btu/bsa/plugin.hpp"
    "${INCLUDE_DIR}/btu/esp/error_code.hpp"
    "${INCLUDE_DIR}/btu/esp/functions.hpp"
    "${INCLUDE_DIR}/btu/hkx/anim.hpp"
    "${INCLUDE_DIR}/btu/hkx/error_code.hpp"
    "${INCLUDE_DIR}/btu/modmanager/mod_folder.hpp"
    "${INCLUDE_DIR}/btu/modmanager/mod_manager.hpp"
    "${INCLUDE_DIR}/btu/nif/detail/common.hpp"
    "${INCLUDE_DIR}/btu/nif/functions.hpp"
    "${INCLUDE_DIR}/btu/nif/mesh.hpp"
    "${INCLUDE_DIR}/btu/nif/optimize.hpp"
    "${INCLUDE_DIR}/btu/tex/error_code.hpp"
    "${INCLUDE_DIR}/btu/tex/compression_device.hpp"
    "${INCLUDE_DIR}/btu/tex/dimension.hpp"
    "${INCLUDE_DIR}/btu/tex/dxtex.hpp"
    "${INCLUDE_DIR}/btu/tex/formats.hpp"
    "${INCLUDE_DIR}/btu/tex/functions.hpp"
    "${INCLUDE_DIR}/btu/tex/optimize.hpp"
    "${INCLUDE_DIR}/btu/tex/texture.hpp"
    "${INCLUDE_DIR}/btu/tex/crunch_texture.hpp"
    "${INCLUDE_DIR}/btu/tex/crunch_functions.hpp"
    "${INCLUDE_DIR}/btu/tex/detail/common.hpp"
    "${INCLUDE_DIR}/btu/tex/detail/formats_string.hpp"
    ../include/btu/common/json.hpp)

set(SOURCE_DIR "${ROOT_DIR}/src")
set(SOURCE_FILES
    "${SOURCE_DIR}/common/filesystem.cpp"
    "${SOURCE_DIR}/common/string.cpp"
    "${SOURCE_DIR}/bsa/archive.cpp"
    "${SOURCE_DIR}/bsa/pack.cpp"
    "${SOURCE_DIR}/bsa/plugin.cpp"
    "${SOURCE_DIR}/bsa/unpack.cpp"
    "${SOURCE_DIR}/esp/functions.cpp"
    "${SOURCE_DIR}/hkx/anim.cpp"
    "${SOURCE_DIR}/modmanager/mod_folder.cpp"
    "${SOURCE_DIR}/modmanager/mod_manager.cpp"
    "${SOURCE_DIR}/nif/functions.cpp"
    "${SOURCE_DIR}/nif/mesh.cpp"
    "${SOURCE_DIR}/nif/optimize.cpp"
    "${SOURCE_DIR}/tex/compression_device.cpp"
    "${SOURCE_DIR}/tex/formats.cpp"
    "${SOURCE_DIR}/tex/functions.cpp"
    "${SOURCE_DIR}/tex/functions_compress_bc7.cpp"
    "${SOURCE_DIR}/tex/optimize.cpp"
    "${SOURCE_DIR}/tex/texture.cpp"
    "${SOURCE_DIR}/tex/crunch_texture.cpp"
    "${SOURCE_DIR}/tex/crunch_functions.cpp"
    "${SOURCE_DIR}/tex/error_code.cpp")

source_group(TREE "${ROOT_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

add_library(
  "${PROJECT_NAME}" STATIC
  ${HEADER_FILES} ${SOURCE_FILES} "${ROOT_DIR}/.clang-format"
  "${ROOT_DIR}/.clang-tidy")
add_library("btu::btu" ALIAS "${PROJECT_NAME}")

find_path(UTF8H_INCLUDE_DIRS "utf8h/utf8.h" REQUIRED)
target_include_directories(
  "${PROJECT_NAME}" SYSTEM
  PUBLIC "$<BUILD_INTERFACE:${UTF8H_INCLUDE_DIRS}>"
         "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

find_package(bsa CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PRIVATE bsa::bsa)

find_path(FLUX_INCLUDE_DIRS "flux.hpp")
target_include_directories("${PROJECT_NAME}" PRIVATE ${FLUX_INCLUDE_DIRS})

find_package(tl-expected CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PRIVATE tl::expected)

find_package(nifly CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PRIVATE nifly)

find_package(directxtex CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PUBLIC Microsoft::DirectXTex)

find_package(bc7enc CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PRIVATE bc7enc::bc7enc)

find_path(BSHOSHANY_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")
target_include_directories("${PROJECT_NAME}"
                           PRIVATE ${BSHOSHANY_THREAD_POOL_INCLUDE_DIRS})

find_package(mpsc_channel CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PRIVATE mpsc::mpsc_channel)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PRIVATE nlohmann_json::nlohmann_json)

find_package(reproc++ CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PRIVATE reproc++)

find_package(crunch2 CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PRIVATE crunch::crunch2)

set_target_properties("${PROJECT_NAME}" PROPERTIES DEBUG_POSTFIX "d")
set_target_properties("${PROJECT_NAME}" PROPERTIES LINKER_LANGUAGE CXX)

target_compile_features("${PROJECT_NAME}" PUBLIC cxx_std_20)

target_compile_definitions(
  "${PROJECT_NAME}" PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

target_compile_definitions(
  "${PROJECT_NAME}" PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
                            _GLIBCXX_PARALLEL) # required for parallel STL
# algorithms on GCC

target_compile_options(
  "${PROJECT_NAME}"
  PRIVATE "$<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-fvisibility=hidden>"
          "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

target_compile_options("${PROJECT_NAME}"
                       PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")

target_include_directories(
  "${PROJECT_NAME}" SYSTEM
  PUBLIC "$<BUILD_INTERFACE:${INCLUDE_DIR}>"
         "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE "$<BUILD_INTERFACE:${INCLUDE_DIR}>"
          "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

install(TARGETS "${PROJECT_NAME}" EXPORT "${PROJECT_NAME}-targets")

install(
  EXPORT "${PROJECT_NAME}-targets"
  NAMESPACE "btu::"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

configure_file("${ROOT_DIR}/cmake/config.cmake.in"
               "${PROJECT_NAME}Config.cmake" @ONLY)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

install(DIRECTORY "${INCLUDE_DIR}/btu"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
