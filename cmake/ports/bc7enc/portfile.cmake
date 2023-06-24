vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  Guekka/bc7enc_rdo
  REF
  2ebcab436faace4e684f52da23e3e465e32874c7
  SHA512
  c212b4e6ba655e428356997684b0e60dadd7afa155570733342d2e672e885dc6d31c6d7fafeb4e4d5cb54fa8f8c482602792774e32c9d40d244e980ffdc6953a
  HEAD_REF
  master)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME bc7enc CONFIG_PATH "lib/cmake/bc7enc")

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include
     ${CURRENT_PACKAGES_DIR}/debug/share)

file(
  INSTALL "${SOURCE_PATH}/LICENSE"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
  RENAME copyright)
