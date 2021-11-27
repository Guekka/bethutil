set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(INCLUDE_DIR "${ROOT_DIR}/tests")
set(HEADER_FILES
    "${INCLUDE_DIR}/bsa/utils.hpp"
    "${INCLUDE_DIR}/tex/utils.hpp")

set(SOURCE_DIR "${ROOT_DIR}/tests")
set(SOURCE_FILES
    "${SOURCE_DIR}/main.cpp"

    "${SOURCE_DIR}/common/filesystem.cpp"
    "${SOURCE_DIR}/common/functional.cpp"
    "${SOURCE_DIR}/common/metaprogramming.cpp"
    "${SOURCE_DIR}/common/path.cpp"
    "${SOURCE_DIR}/common/string.cpp"

    "${SOURCE_DIR}/bsa/archive_data.cpp"
    "${SOURCE_DIR}/bsa/pack.cpp"
    "${SOURCE_DIR}/bsa/plugin.cpp"

    "${SOURCE_DIR}/tex/formats.cpp"
    "${SOURCE_DIR}/tex/functions.cpp"
    "${SOURCE_DIR}/tex/optimize.cpp"

    "${SOURCE_DIR}/hkx/convert.cpp")

if("${BETHUTIL_BUILD_EXAMPLES}")
  list(APPEND SOURCE_FILES)
endif()

source_group("include" FILES ${HEADER_FILES})
source_group(
  TREE "${SOURCE_DIR}"
  PREFIX "src"
  FILES ${SOURCE_FILES})

add_executable(tests ${HEADER_FILES} ${SOURCE_FILES})

target_compile_options(
  tests PRIVATE "$<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-fvisibility=hidden>"
                "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

target_include_directories(
        tests
        PRIVATE
                "${INCLUDE_DIR}"
                "${SOURCE_DIR}"
)

target_link_libraries(
        tests
        PRIVATE
                ${PROJECT_NAME}
                Catch2::Catch2
)

target_compile_options(
        tests
        PUBLIC
        "$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>"
)


find_package(Catch2 CONFIG REQUIRED)
include(Catch)
catch_discover_tests(tests)

macro(acquire_test NAME)
  file(ARCHIVE_EXTRACT INPUT "${ROOT_DIR}/data/${NAME}.7z" DESTINATION
       "${CMAKE_CURRENT_BINARY_DIR}/${NAME}")
endmacro()

acquire_test("read_file")
acquire_test("convert")
acquire_test("convert2")
acquire_test("decompress")
acquire_test("generate_mipmaps")
acquire_test("make_transparent_alpha")
acquire_test("resize")
acquire_test("optimize")
acquire_test("convert-hkx")
acquire_test("exe")

if("${BETHUTIL-TEX_BUILD_EXAMPLES}")
        target_include_directories(
                tests
                PRIVATE
                        "${ROOT_DIR}/examples"
        )
endif()
