#include "config.hpp"

#include <gtest/gtest.h>

using nlohmann::json;

TEST(ParseConfig, DefaultsAndServices) {
    auto const cfg = parse_config(json::parse(R"({"services": ["ssh", {"name": "cron.timer", "title": "Cron"}]})"));
    EXPECT_EQ(cfg.listen_addr, "0.0.0.0");
    EXPECT_EQ(cfg.port, 8080);
    EXPECT_EQ(cfg.log_level, 6);
    EXPECT_EQ(cfg.refresh_sec, 5);
    EXPECT_EQ(cfg.disks, std::vector<std::string>{"/"});
    ASSERT_EQ(cfg.services.size(), 2U);
    EXPECT_EQ(cfg.services[0].name, "ssh.service");
    EXPECT_EQ(cfg.services[0].title, "ssh.service");
    EXPECT_EQ(cfg.services[1].name, "cron.timer");
    EXPECT_EQ(cfg.services[1].title, "Cron");
}

TEST(ParseConfig, ExplicitParams) {
    auto const cfg = parse_config(json::parse(R"({"listen_addr": "127.0.0.1", "port": 9000, "log_level": 3, "refresh_sec": 10, "services": ["a"]})"));
    EXPECT_EQ(cfg.listen_addr, "127.0.0.1");
    EXPECT_EQ(cfg.port, 9000);
    EXPECT_EQ(cfg.log_level, 3);
    EXPECT_EQ(cfg.refresh_sec, 10);
}

TEST(ParseConfig, Errors) {
    EXPECT_THROW((void)parse_config(json::parse("[]")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse("{}")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"services": []})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"services": [""]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"services": [42]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"services": [{"title": "x"}]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"services": [{"name": "a", "title": 1}]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"port": 0, "services": ["a"]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"port": "80", "services": ["a"]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"log_level": 8, "services": ["a"]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"refresh_sec": 0, "services": ["a"]})")), std::runtime_error);
}

TEST(ParseConfig, Groups) {
    auto const cfg = parse_config(json::parse(R"({
        "groups": [
            {"title": "Ядро", "services": ["dpdk-core", {"name": "ngcore-ingress", "title": "Ingress"}]},
            {"title": "Контекст", "services": ["frr@sample"]}
        ],
        "services": ["ssh"]
    })"));
    ASSERT_EQ(cfg.groups.size(), 2U);
    EXPECT_EQ(cfg.groups[0], "Ядро");
    EXPECT_EQ(cfg.groups[1], "Контекст");
    ASSERT_EQ(cfg.services.size(), 4U);
    EXPECT_EQ(cfg.services[0].name, "dpdk-core.service");
    EXPECT_EQ(cfg.services[0].group, "Ядро");
    EXPECT_EQ(cfg.services[1].title, "Ingress");
    EXPECT_EQ(cfg.services[1].group, "Ядро");
    EXPECT_EQ(cfg.services[2].name, "frr@sample.service");
    EXPECT_EQ(cfg.services[2].group, "Контекст");
    EXPECT_EQ(cfg.services[3].name, "ssh.service");
    EXPECT_TRUE(cfg.services[3].group.empty());
}

TEST(ParseConfig, OnlyGroups) {
    auto const cfg = parse_config(json::parse(R"({"groups": [{"title": "A", "services": ["a"]}]})"));
    ASSERT_EQ(cfg.services.size(), 1U);
    EXPECT_EQ(cfg.services[0].group, "A");
}

TEST(ParseConfig, Disks) {
    auto const cfg = parse_config(json::parse(R"({"disks": ["/", "/var"], "services": ["a"]})"));
    EXPECT_EQ(cfg.disks, (std::vector<std::string>{"/", "/var"}));
    EXPECT_THROW((void)parse_config(json::parse(R"({"disks": [], "services": ["a"]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"disks": "/", "services": ["a"]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"disks": ["var"], "services": ["a"]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"disks": [1], "services": ["a"]})")), std::runtime_error);
}

TEST(ParseConfig, GroupErrors) {
    EXPECT_THROW((void)parse_config(json::parse(R"({"groups": {}})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"groups": []})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"groups": ["a"]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"groups": [{"services": ["a"]}]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"groups": [{"title": "", "services": ["a"]}]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"groups": [{"title": "A"}]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"groups": [{"title": "A", "services": []}]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"groups": [{"title": "A", "services": [42]}]})")), std::runtime_error);
    EXPECT_THROW((void)parse_config(json::parse(R"({"groups": [{"title": "A", "services": ["a"]}, {"title": "A", "services": ["b"]}]})")),
                 std::runtime_error);
}

TEST(LoadConfig, MissingFile) { EXPECT_THROW((void)load_config("/nonexistent/cfg.json"), std::runtime_error); }
