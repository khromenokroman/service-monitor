#pragma once
#include <sdbus-c++/IConnection.h>

#include <memory>
#include <mutex>
#include <string>

#include "unit_status.hpp"

/**
 * @brief Клиент systemd через системную шину D-Bus.
 *
 * Запрашивает состояние unit'ов у org.freedesktop.systemd1. Права root не требуются.
 * Методы потокобезопасны: обращения к шине сериализуются мьютексом,
 * так как соединение sd-bus не допускает одновременного использования из нескольких потоков.
 */
class SystemdClient {
   public:
    /**
     * @brief Открывает соединение с системной шиной.
     * @throw sdbus::Error если подключиться к шине не удалось.
     */
    SystemdClient();

    /**
     * @brief Получает состояние unit'а.
     *
     * Ошибки D-Bus не выбрасываются, а записываются в поле UnitStatus::error.
     *
     * @param name Имя unit'а, например "ssh.service".
     * @param title Отображаемое название.
     * @return Состояние unit'а.
     */
    [[nodiscard]] UnitStatus get_status(std::string const &name, std::string const &title);

   private:
    std::mutex m_mutex;                               // 40
    std::unique_ptr<sdbus::IConnection> m_connection; // 8
};
