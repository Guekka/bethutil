cmake_minimum_required(VERSION 3.19)

project(
	bethutil-common
	VERSION 0.1.0
	LANGUAGES CXX
)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

option(BETHUTIL-COMMON_BUILD_SRC "whether we should build the library itself" ON)
if("${BETHUTIL-COMMON_BUILD_SRC}")
	add_subdirectory(src)
endif()

option(BETHUTIL-COMMON_BUILD_EXAMPLES "whether we should build the examples" OFF)
if("${BETHUTIL-COMMON_BUILD_EXAMPLES}")
	add_subdirectory(examples)
endif()

if("${BUILD_TESTING}")
	add_subdirectory(tests)
endif()

option(BETHUTIL-COMMON_BUILD_DOCS "whether we should build documentation" OFF)
if("${BETHUTIL-COMMON_BUILD_DOCS}")
	add_subdirectory(docs)
endif()
