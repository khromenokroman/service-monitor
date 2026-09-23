#include "config.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace {

constexpr std::array<std::string_view, 11> UNIT_SUFFIXES{".service", ".socket", ".timer", ".mount",  ".automount", ".target",
                                                         ".path",    ".slice",  ".scope", ".device", ".swap"};

std::string normalize_unit_name(std::string name) {
    bool const has_suffix = std::ranges::any_of(UNIT_SUFFIXES, [&name](std::string_view s) { return name.ends_with(s); });
    if (!has_suffix) {
        name += ".service";
    }
    return name;
}

ServiceEntry parse_service(nlohmann::json const &j, std::string_view where) {
    ServiceEntry entry;
    if (j.is_string()) {
        entry.name = j.get<std::string>();
    } else if (j.is_object()) {
        if (!j.contains("name") || !j.at("name").is_string()) {
            throw std::runtime_error(fmt::format("{}: поле \"name\" обязательно и должно быть строкой", where));
        }
        entry.name = j.at("name").get<std::string>();
        if (j.contains("title")) {
            if (!j.at("title").is_string()) {
                throw std::runtime_error(fmt::format("{}: поле \"title\" должно быть строкой", where));
            }
            entry.title = j.at("title").get<std::string>();
        }
    } else {
        throw std::runtime_error(fmt::format("{}: ожидается строка или объект", where));
    }

    if (entry.name.empty()) {
        throw std::runtime_error(fmt::format("{}: пустое имя службы", where));
    }
    entry.name = normalize_unit_name(std::move(entry.name));
    if (entry.title.empty()) {
        entry.title = entry.name;
    }
    return entry;
}

void parse_services(nlohmann::json const &j, std::string_view where, std::string const &group, std::vector<ServiceEntry> &out) {
    if (!j.is_array()) {
        throw std::runtime_error(fmt::format("{}: должно быть массивом", where));
    }
    for (std::size_t i = 0; i < j.size(); ++i) {
        auto entry = parse_service(j[i], fmt::format("{}[{}]", where, i));
        entry.group = group;
        out.push_back(std::move(entry));
    }
}

void parse_groups(nlohmann::json const &j, Config &cfg) {
    if (!j.is_array()) {
        throw std::runtime_error("Поле \"groups\" должно быть массивом");
    }
    for (std::size_t i = 0; i < j.size(); ++i) {
        auto const &g = j[i];
        auto const where = fmt::format("groups[{}]", i);
        if (!g.is_object()) {
            throw std::runtime_error(fmt::format("{}: ожидается объект", where));
        }
        if (!g.contains("title") || !g.at("title").is_string() || g.at("title").get<std::string>().empty()) {
            throw std::runtime_error(fmt::format("{}: поле \"title\" обязательно и должно быть непустой строкой", where));
        }
        auto title = g.at("title").get<std::string>();
        if (std::ranges::find(cfg.groups, title) != cfg.groups.end()) {
            throw std::runtime_error(fmt::format("{}: группа \"{}\" уже объявлена", where, title));
        }
        if (!g.contains("services") || !g.at("services").is_array() || g.at("services").empty()) {
            throw std::runtime_error(fmt::format("{}: поле \"services\" обязательно и должно быть непустым массивом", where));
        }
        parse_services(g.at("services"), where + ".services", title, cfg.services);
        cfg.groups.push_back(std::move(title));
    }
}

} // namespace

Config parse_config(nlohmann::json const &j) {
    if (!j.is_object()) {
        throw std::runtime_error("Конфигурация должна быть JSON-объектом");
    }

    Config cfg;
    try {
        cfg.listen_addr = j.value("listen_addr", cfg.listen_addr);
        cfg.port = j.value("port", cfg.port);
        cfg.log_level = j.value("log_level", cfg.log_level);
        cfg.refresh_sec = j.value("refresh_sec", cfg.refresh_sec);
    } catch (nlohmann::json::exception const &ex) {
        throw std::runtime_error(fmt::format("Неверный тип параметра конфигурации: {}", ex.what()));
    }

    if (cfg.port < 1 || cfg.port > 65535) {
        throw std::runtime_error(fmt::format("Неверный порт: {} (допустимо 1-65535)", cfg.port));
    }
    if (cfg.log_level < 0 || cfg.log_level > 7) {
        throw std::runtime_error(fmt::format("Неверный log_level: {} (допустимо 0-7)", cfg.log_level));
    }
    if (cfg.refresh_sec < 1) {
        throw std::runtime_error(fmt::format("Неверный refresh_sec: {} (должен быть не меньше 1)", cfg.refresh_sec));
    }

    if (j.contains("groups")) {
        parse_groups(j.at("groups"), cfg);
    }
    if (j.contains("services")) {
        parse_services(j.at("services"), "services", {}, cfg.services);
    }
    if (cfg.services.empty()) {
        throw std::runtime_error("Не задано ни одной службы: заполните \"services\" и/или \"groups\"");
    }
    return cfg;
}

Config load_config(std::string_view path) {
    std::ifstream file{std::string{path}};
    if (!file.is_open()) {
        auto const err = errno;
        throw std::runtime_error(fmt::format("Не могу открыть настройки({}): {}", path, std::strerror(err)));
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (nlohmann::json::parse_error const &ex) {
        throw std::runtime_error(fmt::format("Ошибка разбора настроек({}): {}", path, ex.what()));
    }
    return parse_config(j);
}
