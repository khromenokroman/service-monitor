#include <cstdlib>
#include <iostream>

#include "config.hpp"

int main(int argc, char *argv[]) {
    try {
        std::string_view const cfg_path = argc > 1 ? argv[1] : "/etc/service-monitor/cfg.json";
        auto const cfg = load_config(cfg_path);
        for (auto const &s : cfg.services) {
            std::cout << s.name << " (" << s.title << ")\n";
        }
    } catch (std::exception const &ex) {
        std::cerr << "Ошибка во время выполнения: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
