vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  tcbrindle/libflow
  REF
  ac56cc60a482ca309dc2c48d2ce13c093bfd74ea
  SHA512
  ee1aca8688dab256d5d79aa41d836558e4171cdb4df8d96efca9c3338ec274bd3fa5beb988adeabee4f9124935bcea945ea2465d80ad3b25edc41e3242313a18
  HEAD_REF
  master)

# Put the licence file where vcpkg expects it
file(COPY ${CMAKE_CURRENT_LIST_DIR}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/libflow)
file(RENAME ${CURRENT_PACKAGES_DIR}/share/libflow/LICENSE ${CURRENT_PACKAGES_DIR}/share/libflow/copyright)

# Copy header files
file(INSTALL ${SOURCE_PATH}/include DESTINATION ${CURRENT_PACKAGES_DIR} FILES_MATCHING PATTERN "*.hpp")
