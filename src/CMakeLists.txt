set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(INCLUDE_DIR "${ROOT_DIR}/include")
set(HEADER_FILES
    "${INCLUDE_DIR}/btu/common.hpp"

    "${INCLUDE_DIR}/btu/common/algorithms.hpp"
    "${INCLUDE_DIR}/btu/common/filesystem.hpp"
    "${INCLUDE_DIR}/btu/common/games.hpp"
    "${INCLUDE_DIR}/btu/common/metaprogramming.hpp"
    "${INCLUDE_DIR}/btu/common/path.hpp"
    "${INCLUDE_DIR}/btu/common/string.hpp"
)

set(SOURCE_DIR "${ROOT_DIR}/src")
set(SOURCE_FILES
    "${SOURCE_DIR}/string.cpp"
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
add_library("btu::common" ALIAS "${PROJECT_NAME}")

find_path(UTF8H_INCLUDE_DIRS "utf8h/utf8.h")
target_include_directories("${PROJECT_NAME}" PRIVATE ${UTF8H_INCLUDE_DIRS})

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
        NAMESPACE "btu::"
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
        DIRECTORY "${INCLUDE_DIR}/btu/common"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/btu"
)

install(
	FILES ${NATVIS_FILES}
	DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}"
)
