#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief Описание отслеживаемой службы.
 */
struct ServiceEntry {
    std::string name;  ///< Имя unit'а systemd, например "ssh.service".
    std::string title; ///< Отображаемое название; если не задано, совпадает с именем unit'а.
    std::string group; ///< Название группы; пустая строка, если служба не входит в группу.
};

/**
 * @brief Конфигурация приложения.
 *
 * Загружается из JSON-файла и содержит параметры HTTP-сервера,
 * логирования и список отслеживаемых служб. Службы хранятся плоским списком:
 * сначала службы групп в порядке следования групп, затем службы без группы.
 */
struct Config {
    std::string listen_addr{"0.0.0.0"};   ///< Адрес, на котором слушает HTTP-сервер.
    std::vector<ServiceEntry> services{}; ///< Список отслеживаемых служб.
    std::vector<std::string> groups{};    ///< Названия групп в порядке из конфигурации.
    std::vector<std::string> disks{"/"}; ///< Точки монтирования для отображения заполненности дисков.
    int port{8080};                      ///< Порт HTTP-сервера.
    int log_level{6};                    ///< Уровень логирования syslog.
    int refresh_sec{5};                  ///< Период автообновления страницы, секунд.
};

/**
 * @brief Разбирает и проверяет конфигурацию из JSON.
 *
 * Службы задаются в массиве "services" (без группы) и/или в массиве "groups"
 * из объектов {"title": "...", "services": [...]}; хотя бы одна служба обязательна.
 * Служба может быть задана строкой ("ssh") или объектом ({"name": "ssh", "title": "SSH"}).
 * Если в имени службы нет суффикса типа unit'а, добавляется ".service".
 * Необязательный массив "disks" задаёт точки монтирования (абсолютные пути), по умолчанию ["/"].
 *
 * @param j JSON-объект конфигурации.
 * @return Проверенная конфигурация.
 * @throw std::runtime_error при некорректной конфигурации.
 */
[[nodiscard]] Config parse_config(nlohmann::json const &j);

/**
 * @brief Загружает конфигурацию из файла.
 * @param path Путь к JSON-файлу конфигурации.
 * @return Проверенная конфигурация.
 * @throw std::runtime_error если файл не открывается или содержит ошибки.
 */
[[nodiscard]] Config load_config(std::string_view path);
