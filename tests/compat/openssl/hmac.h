#pragma once
#include "evp.h"
#include <cstdint>
#include <string>
#include <vector>

namespace _test_crypto {

inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
inline uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
inline uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
inline uint32_t gam0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
inline uint32_t gam1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

inline std::string sha256_bytes(const unsigned char* data, size_t len) {
    static const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint32_t H[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    uint64_t bitlen = static_cast<uint64_t>(len) * 8;
    std::vector<uint8_t> msg;
    msg.reserve(len + 72);
    for (size_t i = 0; i < len; ++i) msg.push_back(data[i]);
    msg.push_back(0x80);
    while ((msg.size() % 64) != 56) msg.push_back(0x00);
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<uint8_t>((bitlen >> (i * 8)) & 0xFF));
    }

    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t W[64];
        for (int t = 0; t < 16; ++t) {
            W[t] = (static_cast<uint32_t>(msg[chunk + t * 4]) << 24) |
                   (static_cast<uint32_t>(msg[chunk + t * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(msg[chunk + t * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(msg[chunk + t * 4 + 3]));
        }
        for (int t = 16; t < 64; ++t) {
            W[t] = gam1(W[t - 2]) + W[t - 7] + gam0(W[t - 15]) + W[t - 16];
        }

        uint32_t a = H[0], b = H[1], c = H[2], d = H[3];
        uint32_t e = H[4], f = H[5], g = H[6], h = H[7];

        for (int t = 0; t < 64; ++t) {
            uint32_t T1 = h + sig1(e) + ch(e, f, g) + K[t] + W[t];
            uint32_t T2 = sig0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }

        H[0] += a; H[1] += b; H[2] += c; H[3] += d;
        H[4] += e; H[5] += f; H[6] += g; H[7] += h;
    }

    std::string digest(32, '\0');
    for (int i = 0; i < 8; ++i) {
        digest[i * 4]     = static_cast<char>((H[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = static_cast<char>((H[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = static_cast<char>((H[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = static_cast<char>(H[i] & 0xFF);
    }
    return digest;
}

} // namespace _test_crypto

inline unsigned char* HMAC(const EVP_MD*, const void* key, int key_len,
                           const unsigned char* data, size_t data_len,
                           unsigned char* md, unsigned int* md_len) {
    static thread_local unsigned char static_buf[32];
    unsigned char* out = md ? md : static_buf;

    std::string k(reinterpret_cast<const char*>(key), key_len);
    if (k.size() > 64) {
        k = _test_crypto::sha256_bytes(reinterpret_cast<const unsigned char*>(k.data()), k.size());
    }
    k.resize(64, '\0');

    std::string o_key_pad(64, '\0');
    std::string i_key_pad(64, '\0');
    for (size_t i = 0; i < 64; ++i) {
        o_key_pad[i] = static_cast<char>(static_cast<unsigned char>(k[i]) ^ 0x5c);
        i_key_pad[i] = static_cast<char>(static_cast<unsigned char>(k[i]) ^ 0x36);
    }

    std::string inner_data = i_key_pad;
    inner_data.append(reinterpret_cast<const char*>(data), data_len);
    std::string inner_hash = _test_crypto::sha256_bytes(
        reinterpret_cast<const unsigned char*>(inner_data.data()), inner_data.size());

    std::string outer_data = o_key_pad;
    outer_data.append(inner_hash);
    std::string outer_hash = _test_crypto::sha256_bytes(
        reinterpret_cast<const unsigned char*>(outer_data.data()), outer_data.size());

    for (size_t i = 0; i < 32; ++i) {
        out[i] = static_cast<unsigned char>(outer_hash[i]);
    }
    if (md_len) *md_len = 32;
    return out;
}
