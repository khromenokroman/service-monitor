#include "unit_status.hpp"

LEVEL status_level(UnitStatus const &s) {
    if (!s.error.empty()) {
        return LEVEL::UNKNOWN;
    }
    if (s.load_state != "loaded") {
        return LEVEL::FAIL;
    }
    if (s.active_state == "active") {
        return LEVEL::OK;
    }
    if (s.active_state == "activating" || s.active_state == "deactivating" || s.active_state == "reloading" || s.active_state == "refreshing") {
        return LEVEL::WARN;
    }
    if (s.active_state == "inactive" || s.active_state == "failed") {
        return LEVEL::FAIL;
    }
    return LEVEL::UNKNOWN;
}

std::string_view level_name(LEVEL level) {
    switch (level) {
        case LEVEL::OK:
            return "ok";
        case LEVEL::WARN:
            return "warn";
        case LEVEL::FAIL:
            return "fail";
        case LEVEL::UNKNOWN:
            break;
    }
    return "unknown";
}

void to_json(nlohmann::json &j, UnitStatus const &s) {
    j = nlohmann::json{
        {"name", s.name},
        {"title", s.title},
        {"description", s.description},
        {"load_state", s.load_state},
        {"active_state", s.active_state},
        {"sub_state", s.sub_state},
        {"error", s.error},
        {"active_enter_us", s.active_enter_us},
        {"memory_bytes", s.memory_bytes},
        {"main_pid", s.main_pid},
        {"n_restarts", s.n_restarts},
        {"level", level_name(status_level(s))},
    };
}
