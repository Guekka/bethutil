vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  vector-of-bool/neo-fun
  REF
  d2de41281094b2be4a3979dd6e5a833d047b3b93
  SHA512
  cbeaffa58fbe65ff0249821a36de3cdf6ffb31ac7236e69b7715057eeac6dea9e72716943cf38922d01523e3b6982de3a6ae90f2e31896c4b0a0050a56b8f3dc
  HEAD_REF
  master)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/config.cmake.in DESTINATION ${SOURCE_PATH})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})

file(
  INSTALL "${SOURCE_PATH}/LICENSE.txt"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
  RENAME copyright)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME neo-fun CONFIG_PATH "lib/cmake/neo-fun")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
