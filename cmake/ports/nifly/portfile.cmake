vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  ousnius/nifly
  REF
  04a2e87dbabf0b279b872f11ad65a2c4b10ec9c4
  SHA512
  dcd11bbc8bb60cb81c6c9e2299960b39467c5b924bce5611867b04687448eff020b9f8f04538d509f3b5d142956b92b61a3d4d9fd9bb375764bde7ae1357798d
  HEAD_REF
  main)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}" OPTIONS -DBUILD_TESTING=OFF)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH "cmake/")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include"
     "${CURRENT_PACKAGES_DIR}/debug/share")

file(
  INSTALL "${SOURCE_PATH}/LICENSE"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
  RENAME copyright)
