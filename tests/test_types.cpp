#include "test_framework.hpp"
#include "peregrine/types.hpp"
#include <fstream>

using namespace peregrine;

TEST_CASE(TypesSuite, GuessMimeTypes) {
    EXPECT_EQ(guess_mime_type("index.html"), "text/html; charset=utf-8");
    EXPECT_EQ(guess_mime_type("style.css"), "text/css; charset=utf-8");
    EXPECT_EQ(guess_mime_type("app.js"), "application/javascript; charset=utf-8");
    EXPECT_EQ(guess_mime_type("module.mjs"), "application/javascript; charset=utf-8");
    EXPECT_EQ(guess_mime_type("data.json"), "application/json");
    EXPECT_EQ(guess_mime_type("image.png"), "image/png");
    EXPECT_EQ(guess_mime_type("PHOTO.JPG"), "image/jpeg");
    EXPECT_EQ(guess_mime_type("photo.jpeg"), "image/jpeg");
    EXPECT_EQ(guess_mime_type("icon.svg"), "image/svg+xml");
    EXPECT_EQ(guess_mime_type("anim.gif"), "image/gif");
    EXPECT_EQ(guess_mime_type("pic.webp"), "image/webp");
    EXPECT_EQ(guess_mime_type("doc.pdf"), "application/pdf");
    EXPECT_EQ(guess_mime_type("app.wasm"), "application/wasm");
    EXPECT_EQ(guess_mime_type("video.mp4"), "video/mp4");
    EXPECT_EQ(guess_mime_type("archive.zip"), "application/zip");
    EXPECT_EQ(guess_mime_type("unknown.xyz123"), "application/octet-stream");
    EXPECT_EQ(guess_mime_type("no_extension"), "application/octet-stream");
}

TEST_CASE(TypesSuite, UploadedFileStructure) {
    UploadedFile f;
    EXPECT_TRUE(f.empty());
    EXPECT_EQ(f.size(), static_cast<size_t>(0));

    f.field_name = "avatar";
    f.filename = "avatar.png";
    f.content_type = "image/png";
    f.data = "FAKE_BINARY_PNG_DATA_12345";

    EXPECT_FALSE(f.empty());
    EXPECT_EQ(f.size(), static_cast<size_t>(26));
    EXPECT_EQ(f.field_name, "avatar");
    EXPECT_EQ(f.filename, "avatar.png");
    EXPECT_EQ(f.content_type, "image/png");

    std::string tmp_path = "test_upload_output.tmp";
    EXPECT_TRUE(f.save(tmp_path));

    std::ifstream in(tmp_path, std::ios::binary);
    EXPECT_TRUE(in.is_open());
    std::string read_back((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(read_back, f.data);
    in.close();
    std::remove(tmp_path.c_str());
}

TEST_CASE(TypesSuite, ServerConfigDefaults) {
    ServerConfig cfg;
    EXPECT_TRUE(cfg.protocol == Protocol::HTTP);
    EXPECT_EQ(cfg.host, "0.0.0.0");
    EXPECT_EQ(cfg.port, 5000);
    EXPECT_EQ(cfg.backlog, 128);
    EXPECT_EQ(cfg.max_body_size, static_cast<size_t>(16 * 1024 * 1024));
    EXPECT_EQ(cfg.thread_pool_size, static_cast<size_t>(8));
    EXPECT_EQ(cfg.max_queue_size, static_cast<size_t>(64));
    EXPECT_EQ(cfg.socket_timeout_seconds, 10);
}
