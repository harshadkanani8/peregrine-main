#include "test_framework.hpp"
#include "peregrine/config.hpp"

using namespace peregrine;

TEST_CASE(ConfigSuite, DefaultSettings) {
    Config cfg;
    EXPECT_EQ(cfg.get("SESSION_COOKIE_NAME"), "session");
    EXPECT_EQ(cfg.get("PERMANENT_SESSION_LIFETIME"), "86400");
    EXPECT_EQ(cfg.get_int("PERMANENT_SESSION_LIFETIME"), 86400);
    EXPECT_TRUE(cfg.get_bool("SESSION_COOKIE_HTTPONLY"));
    EXPECT_FALSE(cfg.get_bool("SESSION_COOKIE_SECURE"));
    EXPECT_EQ(cfg.get("SESSION_COOKIE_SAMESITE"), "Lax");
}

TEST_CASE(ConfigSuite, CustomValuesAndMutations) {
    Config cfg;
    cfg.set("PORT", "8080");
    cfg["DEBUG"] = "true";
    cfg["APP_TITLE"] = "Peregrine App";

    EXPECT_EQ(cfg.get("PORT"), "8080");
    EXPECT_EQ(cfg.get_int("PORT"), 8080);
    EXPECT_TRUE(cfg.get_bool("DEBUG"));
    EXPECT_EQ(cfg["APP_TITLE"], "Peregrine App");
}

TEST_CASE(ConfigSuite, FallbackDefaults) {
    Config cfg;
    EXPECT_EQ(cfg.get("NON_EXISTENT", "default_val"), "default_val");
    EXPECT_EQ(cfg.get_int("NON_EXISTENT", 999), 999);
    EXPECT_TRUE(cfg.get_bool("NON_EXISTENT", true));
    EXPECT_FALSE(cfg.get_bool("NON_EXISTENT", false));
}

TEST_CASE(ConfigSuite, BooleanParsingVariations) {
    Config cfg;
    cfg["b1"] = "1";
    cfg["b2"] = "true";
    cfg["b3"] = "True";
    cfg["b4"] = "YES";
    cfg["b5"] = "yes";
    cfg["b6"] = "0";
    cfg["b7"] = "false";
    cfg["b8"] = "NO";

    EXPECT_TRUE(cfg.get_bool("b1"));
    EXPECT_TRUE(cfg.get_bool("b2"));
    EXPECT_TRUE(cfg.get_bool("b3"));
    EXPECT_TRUE(cfg.get_bool("b4"));
    EXPECT_TRUE(cfg.get_bool("b5"));
    EXPECT_FALSE(cfg.get_bool("b6"));
    EXPECT_FALSE(cfg.get_bool("b7"));
    EXPECT_FALSE(cfg.get_bool("b8"));
}
