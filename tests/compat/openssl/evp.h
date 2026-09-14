#pragma once
#include <cstddef>

struct EVP_MD {};
inline const EVP_MD* EVP_sha256() {
    static EVP_MD md;
    return &md;
}
