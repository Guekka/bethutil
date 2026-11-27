set(install_command "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix
                    "${PACKAGE_PREFIX}")
if(CONFIGURATION)
  list(APPEND install_command --config "${CONFIGURATION}")
endif()

execute_process(
  COMMAND ${install_command}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error)
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "bethutil install failed:\n${output}\n${error}")
endif()

set(configure_command
    "${CMAKE_COMMAND}" -S "${CONSUMER_SOURCE_DIR}" -B "${CONSUMER_BUILD_DIR}"
    -G "${CMAKE_GENERATOR}" "-DCMAKE_PREFIX_PATH=${PACKAGE_PREFIX}")
if(CMAKE_GENERATOR_PLATFORM)
  list(APPEND configure_command -A "${CMAKE_GENERATOR_PLATFORM}")
endif()
if(CMAKE_GENERATOR_TOOLSET)
  list(APPEND configure_command -T "${CMAKE_GENERATOR_TOOLSET}")
endif()
if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND configure_command
       "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()
if(VCPKG_INSTALLED_DIR)
  list(APPEND configure_command "-DVCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR}")
endif()
if(VCPKG_TARGET_TRIPLET)
  list(APPEND configure_command
       "-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}")
endif()
if(VCPKG_OVERLAY_PORTS)
  list(APPEND configure_command "-DVCPKG_OVERLAY_PORTS=${VCPKG_OVERLAY_PORTS}")
endif()
if(VCPKG_MANIFEST_FEATURES)
  list(APPEND configure_command
       "-DVCPKG_MANIFEST_FEATURES=${VCPKG_MANIFEST_FEATURES}")
endif()
if(CMAKE_CXX_COMPILER)
  list(APPEND configure_command "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
endif()
if(CMAKE_BUILD_TYPE)
  list(APPEND configure_command "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif()
if(CMAKE_CXX_FLAGS)
  list(APPEND configure_command "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")
endif()
if(CMAKE_EXE_LINKER_FLAGS)
  list(APPEND configure_command
       "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}")
endif()

execute_process(
  COMMAND ${configure_command}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error)
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "consumer configure failed:\n${output}\n${error}")
endif()

set(build_command "${CMAKE_COMMAND}" --build "${CONSUMER_BUILD_DIR}")
if(CONFIGURATION)
  list(APPEND build_command --config "${CONFIGURATION}")
endif()

execute_process(
  COMMAND ${build_command}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error)
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "consumer build failed:\n${output}\n${error}")
endif()
