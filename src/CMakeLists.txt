set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(INCLUDE_DIR "${ROOT_DIR}/include")
set(HEADER_FILES
    "${INCLUDE_DIR}/btu/bsa.hpp"
    "${INCLUDE_DIR}/btu/bsa/BSACreate.hpp"
    "${INCLUDE_DIR}/btu/bsa/BSAExtract.hpp"
    "${INCLUDE_DIR}/btu/bsa/BSATransform.hpp"
    "${INCLUDE_DIR}/btu/bsa/Games.hpp"
    "${INCLUDE_DIR}/btu/bsa/bethutil_bsa.hpp"

    "${INCLUDE_DIR}/btu/bsa/detail/Algorithms.hpp"
    "${INCLUDE_DIR}/btu/bsa/detail/BSA.hpp"
    "${INCLUDE_DIR}/btu/bsa/detail/BSASplit.hpp"
    "${INCLUDE_DIR}/btu/bsa/detail/BSAType.hpp"
    "${INCLUDE_DIR}/btu/bsa/detail/FileTypes.hpp"
    "${INCLUDE_DIR}/btu/bsa/detail/Plugin.hpp"
    "${INCLUDE_DIR}/btu/bsa/detail/archive.hpp"
    "${INCLUDE_DIR}/btu/bsa/detail/common.hpp"
)

set(SOURCE_DIR "${ROOT_DIR}/src")
set(SOURCE_FILES
    "${SOURCE_DIR}/detail/BSA.cpp"
    "${SOURCE_DIR}/detail/BSASplit.cpp"
    "${SOURCE_DIR}/detail/Plugin.cpp"
    "${SOURCE_DIR}/detail/archive.cpp"
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

find_package(bsa CONFIG REQUIRED)
target_link_libraries("${PROJECT_NAME}" PUBLIC bsa::bsa)

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
	PUBLIC
		"$<BUILD_INTERFACE:${INCLUDE_DIR}>"
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
	PRIVATE
		"${SOURCE_DIR}"
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
		"${ROOT_DIR}/cmake/FindLZ4.cmake"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

install(
	DIRECTORY "${INCLUDE_DIR}/btu/template"
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/bethutil"
)

install(
	FILES ${NATVIS_FILES}
	DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}"
)
