vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  Guekka/bsa
  REF
  72e6734b70bcbf422645cdb210daae88220dc583
  SHA512
  68ca642ac5796a8130087ac61980789890bf74c96f193af8f7a98dc6bf8567832b77fe9cab9d8995dfdacdf644146e426f96497f9679c3fc9e9b258c44421a89
  HEAD_REF
  master)

if(VCPKG_TARGET_IS_LINUX)
  message(WARNING "Build ${PORT} requires at least gcc 10.")
endif()

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS FEATURES xmem
                     BSA_SUPPORT_XMEM)

if(BSA_SUPPORT_XMEM)
  vcpkg_fail_port_install(ON_TARGET "LINUX" MESSAGE
                          "XMem support is only available for windows")
endif()

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}" OPTIONS -DBUILD_TESTING=OFF
                      ${FEATURE_OPTIONS})
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME bsa CONFIG_PATH "lib/cmake/bsa")

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include
     ${CURRENT_PACKAGES_DIR}/debug/share)

file(
  INSTALL "${SOURCE_PATH}/LICENSE"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
  RENAME copyright)
