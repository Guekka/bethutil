vcpkg_fail_port_install(ON_TARGET "OSX" "UWP" ON_ARCH "x86")
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  Ryan-rsm-McKenzie/bsa
  REF
  acf094268fcc9cef041eed3bf735ab3f0e04d620
  SHA512
  a37d4769fd11fc1f2597db6a7444565e1d88c471cc424639f629eab9085f0c4f040f11c7c232189cbbd7abc7b85c9f57bdaa58d8b83b54ed232ee67f996b6f15
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
