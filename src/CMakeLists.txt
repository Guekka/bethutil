set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(INCLUDE_DIR "${ROOT_DIR}/include")
set(HEADER_FILES
    "${INCLUDE_DIR}/btu/common.hpp"
    "${INCLUDE_DIR}/btu/common/algorithms.hpp"
    "${INCLUDE_DIR}/btu/common/error.hpp"
    "${INCLUDE_DIR}/btu/common/filesystem.hpp"
    "${INCLUDE_DIR}/btu/common/functional.hpp"
    "${INCLUDE_DIR}/btu/common/games.hpp"
    "${INCLUDE_DIR}/btu/common/metaprogramming.hpp"
    "${INCLUDE_DIR}/btu/common/path.hpp"
    "${INCLUDE_DIR}/btu/common/string.hpp"
    "${INCLUDE_DIR}/btu/bsa/pack.hpp"
    "${INCLUDE_DIR}/btu/bsa/unpack.hpp"
    "${INCLUDE_DIR}/btu/bsa/transform.hpp"
    "${INCLUDE_DIR}/btu/bsa/archive.hpp"
    "${INCLUDE_DIR}/btu/bsa/archive_data.hpp"
    "${INCLUDE_DIR}/btu/bsa/settings.hpp"
    "${INCLUDE_DIR}/btu/bsa/plugin.hpp"
    "${INCLUDE_DIR}/btu/bsa/detail/common.hpp"
    "${INCLUDE_DIR}/btu/bsa/detail/archive_type.hpp")

set(SOURCE_DIR "${ROOT_DIR}/src")
set(SOURCE_FILES
    "${SOURCE_DIR}/common/string.cpp"
    "${SOURCE_DIR}/bsa/archive.cpp"
    "${SOURCE_DIR}/bsa/archive_data.cpp"
    "${SOURCE_DIR}/bsa/pack.cpp"
    "${SOURCE_DIR}/bsa/plugin.cpp"
    "${SOURCE_DIR}/bsa/unpack.cpp"
    "${SOURCE_DIR}/bsa/detail/archive_type.cpp")

option(BETHUTIL_HKX "Build hkx part" OFF)
if(${BETHUTIL_HKX})
  list(APPEND HEADER_FILES "${INCLUDE_DIR}/btu/hkx/convert.hpp")

  list(APPEND SOURCE_FILES "${SOURCE_DIR}/hkx/convert.cpp")
endif()

option(BETHUTIL_TEX "Build tex part" OFF)
if(${BETHUTIL_TEX})
  list(
    APPEND
    HEADER_FILES
    "${INCLUDE_DIR}/btu/tex.hpp"
    "${INCLUDE_DIR}/btu/tex/error_code.hpp"
    "${INCLUDE_DIR}/btu/tex/compression_device.hpp"
    "${INCLUDE_DIR}/btu/tex/dimension.hpp"
    "${INCLUDE_DIR}/btu/tex/formats.hpp"
    "${INCLUDE_DIR}/btu/tex/functions.hpp"
    "${INCLUDE_DIR}/btu/tex/optimize.hpp"
    "${INCLUDE_DIR}/btu/tex/texture.hpp"
    "${INCLUDE_DIR}/btu/tex/detail/common.hpp"
    "${INCLUDE_DIR}/btu/tex/detail/formats_string.hpp")

  list(
    APPEND
    SOURCE_FILES
    "${SOURCE_DIR}/tex/compression_device.cpp"
    "${SOURCE_DIR}/tex/formats.cpp"
    "${SOURCE_DIR}/tex/functions.cpp"
    "${SOURCE_DIR}/tex/optimize.cpp"
    "${SOURCE_DIR}/tex/texture.cpp"
    "${SOURCE_DIR}/tex/error_code.cpp")
endif()

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

find_path(LIBFLOW_INCLUDE_DIRS "flow.hpp")
target_include_directories("${PROJECT_NAME}" PRIVATE ${LIBFLOW_INCLUDE_DIRS})
# Necessary for libflow
target_compile_definitions("${PROJECT_NAME}"
                           PUBLIC _SILENCE_CLANG_COROUTINE_MESSAGE)

if(${BETHUTIL_TEX})
  find_package(directxtex CONFIG REQUIRED)
  target_link_libraries("${PROJECT_NAME}" PRIVATE Microsoft::DirectXTex)

  find_package(tl-expected CONFIG REQUIRED)
  target_link_libraries("${PROJECT_NAME}" PRIVATE tl::expected)
endif()

if(${BETHUTIL_HKX})
  find_package(reproc++ CONFIG REQUIRED)
  target_link_libraries("${PROJECT_NAME}" PRIVATE reproc++)
endif()

set_target_properties("${PROJECT_NAME}" PROPERTIES DEBUG_POSTFIX "d")
set_target_properties("${PROJECT_NAME}" PROPERTIES LINKER_LANGUAGE CXX)

# See https://github.com/microsoft/STL/issues/1814#issuecomment-914784312
if(MSVC
   AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.29.30129
   AND CMAKE_VERSION VERSION_GREATER 3.20.3)
  target_compile_features("${PROJECT_NAME}" PUBLIC cxx_std_23)
else()
  target_compile_features("${PROJECT_NAME}" PUBLIC cxx_std_20)
endif()

target_compile_definitions(
  "${PROJECT_NAME}" PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

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
