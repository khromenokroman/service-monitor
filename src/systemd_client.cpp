#include "systemd_client.hpp"

#include <sdbus-c++/sdbus-c++.h>

#include <limits>
#include <map>

namespace {

constexpr char const *SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr char const *SYSTEMD_PATH = "/org/freedesktop/systemd1";
constexpr char const *MANAGER_INTERFACE = "org.freedesktop.systemd1.Manager";
constexpr char const *UNIT_INTERFACE = "org.freedesktop.systemd1.Unit";
constexpr char const *SERVICE_INTERFACE = "org.freedesktop.systemd1.Service";

using Properties = std::map<sdbus::PropertyName, sdbus::Variant>;

template <typename T>
T get_or(Properties const &props, std::string_view name, T def = {}) {
    auto const it = props.find(sdbus::PropertyName{std::string{name}});
    if (it == props.end() || !it->second.containsValueOfType<T>()) {
        return def;
    }
    return it->second.get<T>();
}

} // namespace

SystemdClient::SystemdClient() : m_connection{sdbus::createSystemBusConnection()} {}

UnitStatus SystemdClient::get_status(std::string const &name, std::string const &title) {
    UnitStatus st;
    st.name = name;
    st.title = title;

    std::lock_guard const lock{m_mutex};
    try {
        auto manager = sdbus::createProxy(*m_connection, sdbus::ServiceName{SYSTEMD_SERVICE}, sdbus::ObjectPath{SYSTEMD_PATH});
        sdbus::ObjectPath unit_path;
        manager->callMethod("LoadUnit").onInterface(MANAGER_INTERFACE).withArguments(name).storeResultsTo(unit_path);

        auto unit = sdbus::createProxy(*m_connection, sdbus::ServiceName{SYSTEMD_SERVICE}, std::move(unit_path));
        auto const props = unit->getAllProperties().onInterface(UNIT_INTERFACE);
        st.description = get_or<std::string>(props, "Description");
        st.load_state = get_or<std::string>(props, "LoadState");
        st.active_state = get_or<std::string>(props, "ActiveState");
        st.sub_state = get_or<std::string>(props, "SubState");
        st.active_enter_us = get_or<std::uint64_t>(props, "ActiveEnterTimestamp");

        if (name.ends_with(".service") && st.load_state == "loaded") {
            auto const svc = unit->getAllProperties().onInterface(SERVICE_INTERFACE);
            st.main_pid = get_or<std::uint32_t>(svc, "MainPID");
            st.n_restarts = get_or<std::uint32_t>(svc, "NRestarts");
            auto const mem = get_or<std::uint64_t>(svc, "MemoryCurrent");
            st.memory_bytes = mem == std::numeric_limits<std::uint64_t>::max() ? 0 : mem;
        }
    } catch (sdbus::Error const &ex) {
        st.error = ex.getMessage().empty() ? ex.getName() : ex.getMessage();
    }
    return st;
}
