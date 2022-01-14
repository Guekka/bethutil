cmake_minimum_required(VERSION 3.11)
project(neo-fun VERSION 0.1.0)

set(SOURCE_DIR "src/neo")

set(SOURCE_FILES
    ${SOURCE_DIR}/ad_hoc_range.hpp
    ${SOURCE_DIR}/addressof.hpp
    ${SOURCE_DIR}/any_iterator.hpp
    ${SOURCE_DIR}/any_iterator_fwd.hpp
    ${SOURCE_DIR}/any_range.hpp
    ${SOURCE_DIR}/any_range_fwd.hpp
    ${SOURCE_DIR}/archetypes.hpp
    ${SOURCE_DIR}/arrow_proxy.hpp
    ${SOURCE_DIR}/assert.cpp
    ${SOURCE_DIR}/assert.hpp
    ${SOURCE_DIR}/attrib.hpp
    ${SOURCE_DIR}/co_resource.cpp
    ${SOURCE_DIR}/co_resource.hpp
    ${SOURCE_DIR}/co_resource_fwd.hpp
    ${SOURCE_DIR}/concepts.hpp
    ${SOURCE_DIR}/config-pp.hpp
    ${SOURCE_DIR}/declval.hpp
    ${SOURCE_DIR}/enum.hpp
    ${SOURCE_DIR}/error.hpp
    ${SOURCE_DIR}/event.hpp
    ${SOURCE_DIR}/fixed_string.hpp
    ${SOURCE_DIR}/function_traits.hpp
    ${SOURCE_DIR}/fwd.hpp
    ${SOURCE_DIR}/invoke.hpp
    ${SOURCE_DIR}/iterator.hpp
    ${SOURCE_DIR}/iterator_concepts.hpp
    ${SOURCE_DIR}/iterator_facade.hpp
    ${SOURCE_DIR}/map_ref.hpp
    ${SOURCE_DIR}/memory.hpp
    ${SOURCE_DIR}/mutref.hpp
    ${SOURCE_DIR}/opt_ref.hpp
    ${SOURCE_DIR}/opt_ref_fwd.hpp
    ${SOURCE_DIR}/optional.hpp
    ${SOURCE_DIR}/out.hpp
    ${SOURCE_DIR}/overload.hpp
    ${SOURCE_DIR}/platform.hpp
    ${SOURCE_DIR}/pp.hpp
    ${SOURCE_DIR}/range_archetypes.hpp
    ${SOURCE_DIR}/range_concepts.hpp
    ${SOURCE_DIR}/ranges.hpp
    ${SOURCE_DIR}/ref.hpp
    ${SOURCE_DIR}/ref_member.hpp
    ${SOURCE_DIR}/repr.cpp
    ${SOURCE_DIR}/repr.hpp
    ${SOURCE_DIR}/returns.hpp
    ${SOURCE_DIR}/scope.hpp
    ${SOURCE_DIR}/shared.hpp
    ${SOURCE_DIR}/shared_fwd.hpp
    ${SOURCE_DIR}/string.hpp
    ${SOURCE_DIR}/switch_coro.hpp
    ${SOURCE_DIR}/tag.hpp
    ${SOURCE_DIR}/test_concept.hpp
    ${SOURCE_DIR}/text_range.hpp
    ${SOURCE_DIR}/tl.hpp
    ${SOURCE_DIR}/ufmt.cpp
    ${SOURCE_DIR}/ufmt.hpp
    ${SOURCE_DIR}/unref.hpp
    ${SOURCE_DIR}/utf8.hpp
    ${SOURCE_DIR}/utility.hpp
    ${SOURCE_DIR}/version.hpp
    ${SOURCE_DIR}/zstring_view.hpp
    )

add_library("${PROJECT_NAME}" STATIC ${SOURCE_FILES})
add_library("neo::fun" ALIAS "${PROJECT_NAME}")

# See Microsoft STL issue 1814
if(MSVC)
    target_compile_features("${PROJECT_NAME}" PUBLIC cxx_std_23)
else()
    target_compile_features("${PROJECT_NAME}" PUBLIC cxx_std_20)
endif()

if(MSVC)
    target_compile_options("${PROJECT_NAME}" PRIVATE /utf-8 PRIVATE /Zc:__cplusplus PRIVATE /Zc:preprocessor)
endif()

target_include_directories(
    "${PROJECT_NAME}" SYSTEM
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

install(TARGETS "${PROJECT_NAME}" EXPORT "${PROJECT_NAME}-targets")

install(
    EXPORT "${PROJECT_NAME}-targets"
    NAMESPACE "neo::"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

configure_file("config.cmake.in"
    "${PROJECT_NAME}Config.cmake" @ONLY)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

install(DIRECTORY "src/neo"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
