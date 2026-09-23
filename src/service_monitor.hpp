#pragma once
#include <httplib.h>

#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>

#include "config.hpp"
#include "systemd_client.hpp"

/**
 * @brief Веб-приложение мониторинга служб.
 *
 * Отвечает за:
 * - обработку HTTP-запросов (страница и JSON API);
 * - опрос состояния служб через SystemdClient;
 * - запись изменений состояния служб в syslog.
 */
class ServiceMonitor {
   public:
    /**
     * @brief Конструктор приложения.
     *
     * Настраивает логирование и подключается к системной шине D-Bus.
     *
     * @param config Проверенная конфигурация.
     */
    explicit ServiceMonitor(Config config);

    /**
     * @brief Регистрирует маршруты и запускает HTTP-сервер.
     *
     * Блокирует поток до вызова stop().
     *
     * @throw std::runtime_error если не удалось занять адрес и порт.
     */
    void run();

    /**
     * @brief Останавливает HTTP-сервер. Может вызываться из другого потока.
     */
    void stop();

   private:
    /**
     * @brief Опрашивает все службы из конфигурации.
     * @return JSON-ответ для /api/status.
     */
    [[nodiscard]] nlohmann::json collect_status();

    /**
     * @brief Пишет в syslog изменения оценки состояния службы.
     * @param s Текущее состояние службы.
     */
    void log_level_change(UnitStatus const &s);

    httplib::Server m_server;                   // 824
    Config m_config;                            // 72
    std::map<std::string, LEVEL> m_last_levels; // 48
    SystemdClient m_client;                     // 48
    std::mutex m_levels_mutex;                  // 40
};
