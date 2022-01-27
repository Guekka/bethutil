vcpkg_fail_port_install(ON_TARGET "OSX" "UWP" ON_ARCH "x86")
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  Ryan-rsm-McKenzie/bsa
  REF
  18478868baa2b4756cfd0907698ef6d98c118092
  SHA512
  0a7c2f630337181a9508f4c615fca3fd0c4080607254e1ba42b98d57a8182a11fb9e416e1dcaf466e209fcc1a37937919a0b521d31941e493b1bbeb7316f7649
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
