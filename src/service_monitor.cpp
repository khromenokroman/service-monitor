#include "service_monitor.hpp"

#include <fmt/format.h>
#include <syslog.h>
#include <unistd.h>

#include <chrono>
#include <climits>
#include <stdexcept>

#include "index_page.hpp"

namespace {

std::string hostname() {
    std::array<char, HOST_NAME_MAX + 1> buf{};
    if (gethostname(buf.data(), buf.size() - 1) != 0) {
        return {};
    }
    return buf.data();
}

std::uint64_t now_us() {
    using namespace std::chrono;
    return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

} // namespace

ServiceMonitor::ServiceMonitor(Config config) : m_config{std::move(config)} {
    openlog("service-monitor", LOG_PID | LOG_CONS, LOG_USER);
    setlogmask(LOG_UPTO(m_config.log_level));
    syslog(LOG_INFO, "Отслеживается служб: %zu", m_config.services.size());
}

void ServiceMonitor::run() {
    m_server.Get("/", [](httplib::Request const &req, httplib::Response &res) {
        syslog(LOG_DEBUG, "Поступил запрос('/') от %s:%d", req.remote_addr.c_str(), req.remote_port);
        res.set_content(index_page().data(), index_page().size(), "text/html; charset=utf-8");
    });

    m_server.Get("/api/status", [this](httplib::Request const &req, httplib::Response &res) {
        syslog(LOG_DEBUG, "Поступил запрос('/api/status') от %s:%d", req.remote_addr.c_str(), req.remote_port);
        res.set_header("Cache-Control", "no-store");
        res.set_content(collect_status().dump(), "application/json; charset=utf-8");
    });

    m_server.set_socket_options([](socket_t sock) {
        int const opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    });
    if (!m_server.bind_to_port(m_config.listen_addr, m_config.port)) {
        auto const msg = fmt::format("Не удалось занять адрес {}:{}", m_config.listen_addr, m_config.port);
        syslog(LOG_ERR, "%s", msg.c_str());
        throw std::runtime_error(msg);
    }
    syslog(LOG_NOTICE, "Запущен сервер на http://%s:%d", m_config.listen_addr.c_str(), m_config.port);
    m_server.listen_after_bind();
    syslog(LOG_NOTICE, "Сервер остановлен");
}

void ServiceMonitor::stop() { m_server.stop(); }

nlohmann::json ServiceMonitor::collect_status() {
    auto services = nlohmann::json::array();
    for (auto const &entry : m_config.services) {
        auto st = m_client.get_status(entry.name, entry.title);
        st.group = entry.group;
        log_level_change(st);
        services.push_back(st);
    }
    return {
        {"hostname", hostname()},          {"time_us", now_us()}, {"refresh_sec", m_config.refresh_sec}, {"groups", m_config.groups},
        {"services", std::move(services)},
    };
}

void ServiceMonitor::log_level_change(UnitStatus const &s) {
    auto const level = status_level(s);
    std::lock_guard const lock{m_levels_mutex};
    auto const [it, inserted] = m_last_levels.try_emplace(s.name, level);
    if (!inserted && it->second == level) {
        return;
    }
    auto const prev = inserted ? std::string_view{"-"} : level_name(it->second);
    it->second = level;
    int const prio = level == LEVEL::OK ? LOG_NOTICE : LOG_WARNING;
    syslog(prio, "Служба %s: %.*s -> %.*s (%s/%s)%s%s", s.name.c_str(), static_cast<int>(prev.size()), prev.data(),
           static_cast<int>(level_name(level).size()), level_name(level).data(), s.active_state.c_str(), s.sub_state.c_str(),
           s.error.empty() ? "" : ": ", s.error.c_str());
}
