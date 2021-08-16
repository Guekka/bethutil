set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(INCLUDE_DIR "${ROOT_DIR}/tests")
set(HEADER_FILES

)

set(SOURCE_DIR "${ROOT_DIR}/tests")
set(SOURCE_FILES
	"${SOURCE_DIR}/main.cpp"
)

if("${BETHUTIL-BSA_BUILD_EXAMPLES}")
	list(APPEND SOURCE_FILES
	)
endif()

source_group("include" FILES ${HEADER_FILES})
source_group(TREE "${SOURCE_DIR}" PREFIX "src" FILES ${SOURCE_FILES})

add_executable(
	tests
	${HEADER_FILES}
	${SOURCE_FILES}
)
catch_discover_tests(tests)

target_compile_definitions(
	tests
	PRIVATE
		_CRT_SECURE_NO_WARNINGS
		TESTING=1
)

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

macro(acquire_test NAME)
	file(ARCHIVE_EXTRACT
		INPUT "${ROOT_DIR}/data/${NAME}.7z"
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${NAME}"
	)
endmacro()


if("${BETHUTIL-BSA_BUILD_EXAMPLES}")
	target_include_directories(
		tests
		PRIVATE
			"${ROOT_DIR}/examples"
	)
endif()
