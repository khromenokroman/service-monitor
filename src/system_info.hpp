#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief Счётчики времени процессора из /proc/stat.
 */
struct CpuTimes {
    std::uint64_t busy{};  ///< Время работы (всё, кроме idle и iowait), в тиках.
    std::uint64_t total{}; ///< Полное время, в тиках.
};

/**
 * @brief Разбирает /proc/stat.
 * @param text Содержимое /proc/stat.
 * @return Счётчики: элемент 0 — суммарно по всем ядрам, далее по одному на каждое ядро.
 */
[[nodiscard]] std::vector<CpuTimes> parse_proc_stat(std::string_view text);

/**
 * @brief Вычисляет загрузку процессора между двумя замерами.
 * @param prev Предыдущий замер.
 * @param cur Текущий замер.
 * @return Загрузка в процентах для каждого элемента; пустой вектор, если число ядер изменилось.
 */
[[nodiscard]] std::vector<double> cpu_usage(std::vector<CpuTimes> const &prev, std::vector<CpuTimes> const &cur);

/**
 * @brief Разбирает /proc/meminfo или /sys/devices/system/node/nodeN/meminfo.
 *
 * Префикс "Node N " отбрасывается, значения в kB переводятся в байты.
 *
 * @param text Содержимое файла.
 * @return Значения по именам полей.
 */
[[nodiscard]] std::map<std::string, std::uint64_t> parse_meminfo(std::string_view text);

/**
 * @brief Разбирает /proc/loadavg.
 * @param text Содержимое /proc/loadavg.
 * @return Средняя загрузка за 1, 5 и 15 минут.
 */
[[nodiscard]] std::array<double, 3> parse_loadavg(std::string_view text);

/**
 * @brief Сборщик метрик системы: процессор, NUMA, память, swap, hugepages, диски.
 *
 * Данные читаются из /proc, /sys и statvfs, права root не требуются.
 * Загрузка процессора считается по разнице счётчиков между вызовами collect().
 */
class SystemInfo {
   public:
    /**
     * @brief Конструктор. Делает первый замер счётчиков процессора.
     * @param disks Точки монтирования, заполненность которых нужно показывать.
     */
    explicit SystemInfo(std::vector<std::string> disks);

    /**
     * @brief Собирает метрики системы. Потокобезопасен.
     * @return JSON-объект с метриками.
     */
    [[nodiscard]] nlohmann::json collect();

   private:
    /**
     * @brief Обновляет загрузку процессора, если с прошлого замера прошло достаточно времени.
     */
    void update_cpu_usage();

    std::mutex m_mutex;                                    // 40
    std::vector<std::string> m_disks;                      // 24
    std::vector<CpuTimes> m_prev_cpu;                      // 24
    std::vector<double> m_cpu_usage;                       // 24
    std::chrono::steady_clock::time_point m_prev_cpu_time; // 8
};
