#include "system_info.hpp"

#include <sys/statvfs.h>
#include <sys/utsname.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace {

constexpr auto MIN_CPU_INTERVAL = std::chrono::milliseconds{500};
constexpr std::string_view NUMA_DIR{"/sys/devices/system/node"};

std::string read_file(std::string const &path) {
    std::ifstream f{path};
    if (!f.is_open()) {
        return {};
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string trim(std::string s) {
    auto const not_space = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::ranges::find_if(s, not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}

std::string cpu_model() {
    std::istringstream in{read_file("/proc/cpuinfo")};
    for (std::string line; std::getline(in, line);) {
        if (line.starts_with("model name")) {
            auto const pos = line.find(':');
            return pos == std::string::npos ? std::string{} : trim(line.substr(pos + 1));
        }
    }
    return {};
}

std::string kernel_release() {
    utsname u{};
    return uname(&u) == 0 ? std::string{u.release} : std::string{};
}

nlohmann::json numa_nodes() {
    std::vector<std::pair<int, std::filesystem::path>> nodes;
    std::error_code ec;
    for (auto const &entry : std::filesystem::directory_iterator{NUMA_DIR, ec}) {
        auto const name = entry.path().filename().string();
        int id{};
        if (!name.starts_with("node") || name.size() == 4) {
            continue;
        }
        auto const [ptr, err] = std::from_chars(name.data() + 4, name.data() + name.size(), id);
        if (err != std::errc{} || ptr != name.data() + name.size()) {
            continue;
        }
        nodes.emplace_back(id, entry.path());
    }
    std::ranges::sort(nodes);

    auto result = nlohmann::json::array();
    for (auto const &[id, path] : nodes) {
        auto const mem = parse_meminfo(read_file(path / "meminfo"));
        auto const get = [&mem](std::string const &k) { return mem.contains(k) ? mem.at(k) : 0; };
        result.push_back({
            {"node", id},
            {"cpus", trim(read_file(path / "cpulist"))},
            {"mem_total", get("MemTotal")},
            {"mem_free", get("MemFree")},
        });
    }
    return result;
}

nlohmann::json disk_usage(std::string const &path) {
    struct statvfs st {};
    if (statvfs(path.c_str(), &st) != 0) {
        return {{"path", path}, {"error", std::generic_category().message(errno)}};
    }
    auto const bs = static_cast<std::uint64_t>(st.f_frsize);
    return {
        {"path", path},
        {"total", st.f_blocks * bs},
        {"used", (st.f_blocks - st.f_bfree) * bs},
        {"avail", st.f_bavail * bs},
    };
}

} // namespace

std::vector<CpuTimes> parse_proc_stat(std::string_view text) {
    std::vector<CpuTimes> result;
    std::istringstream in{std::string{text}};
    for (std::string line; std::getline(in, line);) {
        if (!line.starts_with("cpu")) {
            continue;
        }
        std::istringstream fields{line};
        std::string label;
        fields >> label;
        // user nice system idle iowait irq softirq steal; guest уже учтён в user
        std::array<std::uint64_t, 8> v{};
        for (auto &x : v) {
            if (!(fields >> x)) {
                x = 0;
            }
        }
        std::uint64_t total{};
        for (auto const x : v) {
            total += x;
        }
        result.push_back({total - v[3] - v[4], total});
    }
    return result;
}

std::vector<double> cpu_usage(std::vector<CpuTimes> const &prev, std::vector<CpuTimes> const &cur) {
    if (prev.size() != cur.size()) {
        return {};
    }
    std::vector<double> result;
    result.reserve(cur.size());
    for (std::size_t i = 0; i < cur.size(); ++i) {
        auto const total = cur[i].total > prev[i].total ? cur[i].total - prev[i].total : 0;
        auto const busy = cur[i].busy > prev[i].busy ? cur[i].busy - prev[i].busy : 0;
        result.push_back(total == 0 ? 0.0 : std::min(100.0, 100.0 * static_cast<double>(busy) / static_cast<double>(total)));
    }
    return result;
}

std::map<std::string, std::uint64_t> parse_meminfo(std::string_view text) {
    std::map<std::string, std::uint64_t> result;
    std::istringstream in{std::string{text}};
    for (std::string line; std::getline(in, line);) {
        std::istringstream fields{line};
        std::string key;
        fields >> key;
        if (key == "Node") {
            std::string node;
            fields >> node >> key;
        }
        if (key.empty() || key.back() != ':') {
            continue;
        }
        key.pop_back();
        std::uint64_t value{};
        std::string unit;
        if (!(fields >> value)) {
            continue;
        }
        fields >> unit;
        result[key] = unit == "kB" ? value * 1024 : value;
    }
    return result;
}

std::array<double, 3> parse_loadavg(std::string_view text) {
    std::array<double, 3> result{};
    std::istringstream in{std::string{text}};
    for (auto &x : result) {
        if (!(in >> x)) {
            x = 0;
        }
    }
    return result;
}

SystemInfo::SystemInfo(std::vector<std::string> disks)
    : m_disks{std::move(disks)}, m_prev_cpu{parse_proc_stat(read_file("/proc/stat"))}, m_prev_cpu_time{std::chrono::steady_clock::now()} {}

void SystemInfo::update_cpu_usage() {
    auto const now = std::chrono::steady_clock::now();
    if (!m_cpu_usage.empty() && now - m_prev_cpu_time < MIN_CPU_INTERVAL) {
        return;
    }
    auto cur = parse_proc_stat(read_file("/proc/stat"));
    m_cpu_usage = cpu_usage(m_prev_cpu, cur);
    m_prev_cpu = std::move(cur);
    m_prev_cpu_time = now;
}

nlohmann::json SystemInfo::collect() {
    std::vector<double> usage;
    {
        std::lock_guard const lock{m_mutex};
        update_cpu_usage();
        usage = m_cpu_usage;
    }

    auto const mem = parse_meminfo(read_file("/proc/meminfo"));
    auto const get = [&mem](std::string const &k) { return mem.contains(k) ? mem.at(k) : 0; };
    auto const load = parse_loadavg(read_file("/proc/loadavg"));
    double uptime{};
    std::istringstream{read_file("/proc/uptime")} >> uptime;

    auto disks = nlohmann::json::array();
    for (auto const &d : m_disks) {
        disks.push_back(disk_usage(d));
    }

    return {
        {"kernel", kernel_release()},
        {"cpu_model", cpu_model()},
        {"uptime_sec", static_cast<std::uint64_t>(uptime)},
        {"load", load},
        {"cpu",
         {
             {"cores", usage.empty() ? 0 : usage.size() - 1},
             {"usage", usage.empty() ? 0.0 : usage.front()},
             {"per_core", usage.empty() ? std::vector<double>{} : std::vector<double>(usage.begin() + 1, usage.end())},
         }},
        {"numa", numa_nodes()},
        {"memory", {{"total", get("MemTotal")}, {"available", get("MemAvailable")}}},
        {"swap", {{"total", get("SwapTotal")}, {"free", get("SwapFree")}}},
        {"hugepages", {{"total", get("HugePages_Total")}, {"free", get("HugePages_Free")}, {"size", get("Hugepagesize")}}},
        {"disks", std::move(disks)},
    };
}
