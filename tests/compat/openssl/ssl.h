#pragma once
#include <cstddef>

struct SSL {};
struct SSL_CTX {};
struct SSL_METHOD {};

#define SSL_FILETYPE_PEM 1
#define TLS1_2_VERSION 0x0303
#define TLS1_3_VERSION 0x0304

#define SSL_OP_NO_COMPRESSION 0x00020000L
#define SSL_OP_NO_RENEGOTIATION 0x40000000L
#define SSL_OP_CIPHER_SERVER_PREFERENCE 0x00400000L
#define SSL_OP_SINGLE_ECDH_USE 0x00080000L

inline const SSL_METHOD* TLS_server_method() { static SSL_METHOD m; return &m; }
inline SSL_CTX* SSL_CTX_new(const SSL_METHOD*) { static SSL_CTX ctx; return &ctx; }
inline void SSL_CTX_free(SSL_CTX*) {}
inline int SSL_CTX_use_certificate_file(SSL_CTX*, const char*, int) { return 1; }
inline int SSL_CTX_use_PrivateKey_file(SSL_CTX*, const char*, int) { return 1; }
inline int SSL_CTX_check_private_key(SSL_CTX*) { return 1; }
inline void SSL_CTX_set_min_proto_version(SSL_CTX*, int) {}
inline long SSL_CTX_set_options(SSL_CTX*, long) { return 0; }
inline int SSL_CTX_set1_groups_list(SSL_CTX*, const char*) { return 1; }
inline int SSL_CTX_set1_curves_list(SSL_CTX*, const char*) { return 1; }
inline int SSL_CTX_set_cipher_list(SSL_CTX*, const char*) { return 1; }
inline int SSL_CTX_set_ciphersuites(SSL_CTX*, const char*) { return 1; }
inline void OpenSSL_add_all_algorithms() {}
inline void SSL_load_error_strings() {}
inline SSL* SSL_new(SSL_CTX*) { static SSL s; return &s; }
inline void SSL_set_fd(SSL*, int) {}
inline int SSL_accept(SSL*) { return 1; }
inline int SSL_read(SSL*, char*, int) { return 0; }
inline int SSL_write(SSL*, const char*, int) { return 0; }
inline void SSL_shutdown(SSL*) {}
inline void SSL_free(SSL*) {}
