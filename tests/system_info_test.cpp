#include "system_info.hpp"

#include <gtest/gtest.h>

TEST(SystemInfo, ParseProcStat) {
    auto const t = parse_proc_stat(
        "cpu  100 0 50 800 50 0 0 0 0 0\n"
        "cpu0 60 0 20 400 20 0 0 0 0 0\n"
        "cpu1 40 0 30 400 30 0 0 0 0 0\n"
        "intr 123 4 5\n"
        "ctxt 999\n");
    ASSERT_EQ(t.size(), 3U);
    EXPECT_EQ(t[0].total, 1000U);
    EXPECT_EQ(t[0].busy, 150U);
    EXPECT_EQ(t[1].busy, 80U);
    EXPECT_EQ(t[2].total, 500U);
}

TEST(SystemInfo, CpuUsage) {
    std::vector<CpuTimes> const prev{{100, 1000}, {50, 500}};
    std::vector<CpuTimes> const cur{{150, 1100}, {50, 600}};
    auto const u = cpu_usage(prev, cur);
    ASSERT_EQ(u.size(), 2U);
    EXPECT_DOUBLE_EQ(u[0], 50.0);
    EXPECT_DOUBLE_EQ(u[1], 0.0);
    EXPECT_TRUE(cpu_usage(prev, {{1, 2}}).empty());
    auto const same = cpu_usage(prev, prev);
    EXPECT_DOUBLE_EQ(same[0], 0.0);
}

TEST(SystemInfo, ParseMeminfo) {
    auto const m = parse_meminfo(
        "MemTotal:       16099336 kB\n"
        "MemAvailable:    4442372 kB\n"
        "HugePages_Total:     512\n"
        "Hugepagesize:       2048 kB\n");
    EXPECT_EQ(m.at("MemTotal"), 16099336ULL * 1024);
    EXPECT_EQ(m.at("MemAvailable"), 4442372ULL * 1024);
    EXPECT_EQ(m.at("HugePages_Total"), 512U);
    EXPECT_EQ(m.at("Hugepagesize"), 2048U * 1024);
}

TEST(SystemInfo, ParseNodeMeminfo) {
    auto const m = parse_meminfo(
        "Node 0 MemTotal:       16099336 kB\n"
        "Node 0 MemFree:         1975380 kB\n");
    EXPECT_EQ(m.at("MemTotal"), 16099336ULL * 1024);
    EXPECT_EQ(m.at("MemFree"), 1975380ULL * 1024);
}

TEST(SystemInfo, ParseLoadavg) {
    auto const l = parse_loadavg("1.02 0.74 0.50 1/2821 289471\n");
    EXPECT_DOUBLE_EQ(l[0], 1.02);
    EXPECT_DOUBLE_EQ(l[1], 0.74);
    EXPECT_DOUBLE_EQ(l[2], 0.50);
    auto const bad = parse_loadavg("");
    EXPECT_DOUBLE_EQ(bad[0], 0.0);
}

TEST(SystemInfo, Collect) {
    SystemInfo info{{"/", "/nonexistent-mount"}};
    auto const j = info.collect();
    EXPECT_GT(j.at("cpu").at("cores").get<int>(), 0);
    EXPECT_EQ(j.at("cpu").at("per_core").size(), j.at("cpu").at("cores").get<std::size_t>());
    EXPECT_GT(j.at("memory").at("total").get<std::uint64_t>(), 0U);
    ASSERT_EQ(j.at("disks").size(), 2U);
    EXPECT_GT(j.at("disks")[0].at("total").get<std::uint64_t>(), 0U);
    EXPECT_TRUE(j.at("disks")[1].contains("error"));
}
