set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(INCLUDE_DIR "${ROOT_DIR}/include")
set(HEADER_FILES
    "${INCLUDE_DIR}/btu/bsa/pack.hpp"
    "${INCLUDE_DIR}/btu/bsa/unpack.hpp"
    "${INCLUDE_DIR}/btu/bsa/transform.hpp"

    "${INCLUDE_DIR}/btu/bsa/archive_data.hpp"
    "${INCLUDE_DIR}/btu/bsa/settings.hpp"
    "${INCLUDE_DIR}/btu/bsa/plugin.hpp"

    "${INCLUDE_DIR}/btu/bsa/detail/common.hpp"  
    "${INCLUDE_DIR}/btu/bsa/detail/archive_type.hpp"

    "${INCLUDE_DIR}/btu/bsa/detail/backends/archive.hpp"
    "${INCLUDE_DIR}/btu/bsa/detail/backends/rsm_archive.hpp"
)

set(SOURCE_DIR "${ROOT_DIR}/src")
set(SOURCE_FILES
    "${SOURCE_DIR}/pack.cpp"

    "${SOURCE_DIR}/archive_data.cpp"
    "${SOURCE_DIR}/plugin.cpp"

    "${SOURCE_DIR}/detail/backends/rsm_archive.cpp"
    "${SOURCE_DIR}/detail/backends/rsm_archive_fo4.cpp"
)

source_group(
	TREE "${ROOT_DIR}"
	FILES
		${HEADER_FILES}
		${SOURCE_FILES}
)

add_library(
	"${PROJECT_NAME}"
	STATIC
	${HEADER_FILES}
	${SOURCE_FILES}
	"${ROOT_DIR}/.clang-format"
	"${ROOT_DIR}/.clang-tidy"
)
add_library("btu::bsa" ALIAS "${PROJECT_NAME}")

find_package(rsm-bsa CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PUBLIC rsm-bsa::rsm-bsa)

find_package(bethutil-common CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PUBLIC btu::bethutil-common)

find_package(directx-headers CONFIG REQUIRED)
find_package(directxmath CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PRIVATE Microsoft::DirectXTex)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set_target_properties(
	"${PROJECT_NAME}"
	PROPERTIES
		DEBUG_POSTFIX "d"
)

set_target_properties("${PROJECT_NAME}" PROPERTIES LINKER_LANGUAGE CXX)

target_compile_features(
	"${PROJECT_NAME}"
	PUBLIC
		cxx_std_20
)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang|GNU")
	target_compile_options(
		"${PROJECT_NAME}"
		PRIVATE
			"-fvisibility=hidden"
	)
endif()

target_include_directories(
	"${PROJECT_NAME}"
        SYSTEM PUBLIC
		"$<BUILD_INTERFACE:${INCLUDE_DIR}>"
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        PRIVATE
                "$<BUILD_INTERFACE:${INCLUDE_DIR}>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

install(
	TARGETS "${PROJECT_NAME}"
	EXPORT "${PROJECT_NAME}-targets"
)

install(
	EXPORT "${PROJECT_NAME}-targets"
	NAMESPACE "${PROJECT_NAME}::"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

configure_file(
	"${ROOT_DIR}/cmake/config.cmake.in"
	"${PROJECT_NAME}Config.cmake"
	@ONLY
)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	COMPATIBILITY SameMajorVersion
)

install(
	FILES
		"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

install(
        DIRECTORY "${INCLUDE_DIR}/btu/bsa"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/btu"
)

install(
	FILES ${NATVIS_FILES}
	DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}"
)
