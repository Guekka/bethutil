set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(INCLUDE_DIR "${ROOT_DIR}/tests")
set(HEADER_FILES "${INCLUDE_DIR}/bsa/utils.hpp" "${INCLUDE_DIR}/tex/utils.hpp")

set(SOURCE_DIR "${ROOT_DIR}/tests")
set(SOURCE_FILES
    "${SOURCE_DIR}/utils.hpp"
    "${SOURCE_DIR}/common/filesystem.cpp"
    "${SOURCE_DIR}/common/functional.cpp"
    "${SOURCE_DIR}/common/json.cpp"
    "${SOURCE_DIR}/common/metaprogramming.cpp"
    "${SOURCE_DIR}/common/string.cpp"
    "${SOURCE_DIR}/bsa/archive.cpp"
    "${SOURCE_DIR}/bsa/pack.cpp"
    "${SOURCE_DIR}/bsa/plugin.cpp"
    "${SOURCE_DIR}/bsa/unpack.cpp"
    "${SOURCE_DIR}/esp/functions.cpp"
    "${SOURCE_DIR}/modmanager/mod_folder.cpp"
    "${SOURCE_DIR}/modmanager/mod_manager.cpp"
    "${SOURCE_DIR}/nif/functions.cpp"
    "${SOURCE_DIR}/nif/optimize.cpp"
    "${SOURCE_DIR}/nif/utils.hpp"
    "${SOURCE_DIR}/tex/formats.cpp"
    "${SOURCE_DIR}/tex/functions.cpp"
    "${SOURCE_DIR}/tex/optimize.cpp")

if(${BETHUTIL_HKX})
  list(APPEND SOURCE_FILES "${SOURCE_DIR}/hkx/anim.cpp")
endif()

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

target_include_directories(tests PRIVATE "${INCLUDE_DIR}" "${SOURCE_DIR}")

target_link_libraries(tests PRIVATE ${PROJECT_NAME} Catch2::Catch2WithMain)

target_compile_options(tests
                       PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")

find_package(Catch2 CONFIG REQUIRED)
include(Catch)
catch_discover_tests(tests)

macro(acquire_test NAME)
  file(ARCHIVE_EXTRACT INPUT "${ROOT_DIR}/data/${NAME}.7z" DESTINATION
       "${CMAKE_CURRENT_BINARY_DIR}/${NAME}")
endmacro()

acquire_test("bsa_clean_dummy_plugins")
acquire_test("bsa_unpack")
acquire_test("bsa_load_save")
acquire_test("convert-hkx")
acquire_test("decompress")
acquire_test("esp")
acquire_test("make_transparent_alpha")
acquire_test("modfolder_transform")
acquire_test("optimize")
acquire_test("nif_optimize")
acquire_test("resize")
acquire_test("convert")
acquire_test("convert_bc1")
acquire_test("exe")
acquire_test("mod_manager")
acquire_test("nif_convert")
acquire_test("pack")
acquire_test("convert2")
acquire_test("generate_mipmaps")
acquire_test("modfolder")
acquire_test("nif_corrupted")
acquire_test("read_file")
acquire_test("nif_memory_io")
acquire_test("tex_memory_io")

if("${BETHUTIL_BUILD_EXAMPLES}")
  target_include_directories(tests PRIVATE "${ROOT_DIR}/examples")
endif()
