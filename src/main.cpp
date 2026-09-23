#include <cstdlib>
#include <iostream>

#include "config.hpp"
#include "systemd_client.hpp"

int main(int argc, char *argv[]) {
    try {
        std::string_view const cfg_path = argc > 1 ? argv[1] : "/etc/service-monitor/cfg.json";
        auto const cfg = load_config(cfg_path);
        SystemdClient client;
        auto result = nlohmann::json::array();
        for (auto const &s : cfg.services) {
            result.push_back(client.get_status(s.name, s.title));
        }
        std::cout << result.dump(2) << std::endl;
    } catch (std::exception const &ex) {
        std::cerr << "Ошибка во время выполнения: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
