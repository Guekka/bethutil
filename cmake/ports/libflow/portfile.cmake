vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  Guekka/libflow
  REF
  017b5c208fc7e9802a45160fc28d2be2e93889f6
  SHA512
  bed10d6ff3e9a8cc895a1289b32c949e5835865ad281d2aa9d22fe23098fc1ed6fa94802543cfb55ffa2cc0ea317200f87ff4a55cb0690220868e7af820a4d8e
  HEAD_REF
  master)

# Put the licence file where vcpkg expects it
file(COPY ${CMAKE_CURRENT_LIST_DIR}/LICENSE
     DESTINATION ${CURRENT_PACKAGES_DIR}/share/libflow)
file(RENAME ${CURRENT_PACKAGES_DIR}/share/libflow/LICENSE
     ${CURRENT_PACKAGES_DIR}/share/libflow/copyright)

# Copy header files
file(
  INSTALL ${SOURCE_PATH}/include
  DESTINATION ${CURRENT_PACKAGES_DIR}
  FILES_MATCHING
  PATTERN "*.hpp")
