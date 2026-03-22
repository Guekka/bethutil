vcpkg_from_github(
        OUT_SOURCE_PATH
        SOURCE_PATH
        REPO
        sheredom/utf8.h
        REF
        a6cd7d4329e9336518d2e2a449d4f3cb8def54cc
        SHA512
        265777c433ee9b6ec6cf30cd29728424a53beda7c7ddd3be8230ccdf6748d9f25c6473068a47c1d24e1e9c431097e12aeda5f7ad31a35df5f33d8a1e46af4c79
        HEAD_REF
        master)

file(COPY "${SOURCE_PATH}/utf8.h"
        DESTINATION "${CURRENT_PACKAGES_DIR}/include/utf8h")

file(
        INSTALL "${SOURCE_PATH}/LICENSE"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
        RENAME copyright)
