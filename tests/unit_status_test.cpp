#include "unit_status.hpp"

#include <gtest/gtest.h>

namespace {

UnitStatus make(std::string load, std::string active) {
    UnitStatus s;
    s.load_state = std::move(load);
    s.active_state = std::move(active);
    return s;
}

} // namespace

TEST(StatusLevel, Classification) {
    EXPECT_EQ(status_level(make("loaded", "active")), LEVEL::OK);
    EXPECT_EQ(status_level(make("loaded", "activating")), LEVEL::WARN);
    EXPECT_EQ(status_level(make("loaded", "deactivating")), LEVEL::WARN);
    EXPECT_EQ(status_level(make("loaded", "reloading")), LEVEL::WARN);
    EXPECT_EQ(status_level(make("loaded", "inactive")), LEVEL::FAIL);
    EXPECT_EQ(status_level(make("loaded", "failed")), LEVEL::FAIL);
    EXPECT_EQ(status_level(make("not-found", "inactive")), LEVEL::FAIL);
    EXPECT_EQ(status_level(make("masked", "inactive")), LEVEL::FAIL);
    EXPECT_EQ(status_level(make("loaded", "strange")), LEVEL::UNKNOWN);

    auto s = make("loaded", "active");
    s.error = "Access denied";
    EXPECT_EQ(status_level(s), LEVEL::UNKNOWN);
}

TEST(StatusLevel, ToJson) {
    auto s = make("loaded", "active");
    s.name = "ssh.service";
    s.main_pid = 42;
    nlohmann::json const j = s;
    EXPECT_EQ(j.at("name"), "ssh.service");
    EXPECT_EQ(j.at("main_pid"), 42);
    EXPECT_EQ(j.at("level"), "ok");
}
