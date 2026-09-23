#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

/**
 * @brief Итоговая оценка состояния службы для отображения.
 */
enum class LEVEL {
    OK,      ///< Служба работает.
    WARN,    ///< Служба в переходном состоянии (запуск, остановка, перезагрузка).
    FAIL,    ///< Служба упала, остановлена или не найдена.
    UNKNOWN, ///< Состояние не удалось получить.
};

/**
 * @brief Состояние unit'а systemd, полученное через D-Bus.
 */
struct UnitStatus {
    std::string name;                ///< Имя unit'а.
    std::string title;               ///< Отображаемое название из конфигурации.
    std::string description;         ///< Свойство Description.
    std::string load_state;          ///< Свойство LoadState (loaded, not-found, masked, ...).
    std::string active_state;        ///< Свойство ActiveState (active, inactive, failed, ...).
    std::string sub_state;           ///< Свойство SubState (running, exited, dead, ...).
    std::string error;               ///< Текст ошибки D-Bus, если состояние получить не удалось.
    std::uint64_t active_enter_us{}; ///< ActiveEnterTimestamp, микросекунды от эпохи Unix; 0 если неизвестно.
    std::uint64_t memory_bytes{};    ///< MemoryCurrent, байт; 0 если неизвестно.
    std::uint32_t main_pid{};        ///< MainPID; 0 если процесса нет.
    std::uint32_t n_restarts{};      ///< NRestarts, число автоматических перезапусков.
};

/**
 * @brief Вычисляет итоговую оценку состояния службы.
 * @param s Состояние unit'а.
 * @return Оценка состояния.
 */
[[nodiscard]] LEVEL status_level(UnitStatus const &s);

/**
 * @brief Возвращает строковое имя оценки ("ok", "warn", "fail", "unknown").
 * @param level Оценка состояния.
 * @return Имя оценки.
 */
[[nodiscard]] std::string_view level_name(LEVEL level);

/**
 * @brief Сериализует состояние unit'а в JSON для HTTP API.
 * @param j Результирующий JSON.
 * @param s Состояние unit'а.
 */
void to_json(nlohmann::json &j, UnitStatus const &s);
