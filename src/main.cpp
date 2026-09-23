#include <signal.h>

#include <cstdlib>
#include <iostream>
#include <thread>

#include "config.hpp"
#include "service_monitor.hpp"

int main(int argc, char *argv[]) {
    try {
        std::string_view const cfg_path = argc > 1 ? argv[1] : "/etc/service-monitor/cfg.json";

        sigset_t signals;
        sigemptyset(&signals);
        sigaddset(&signals, SIGINT);
        sigaddset(&signals, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &signals, nullptr);

        ServiceMonitor monitor{load_config(cfg_path)};
        std::jthread signal_thread{[&monitor, &signals] {
            int sig{};
            sigwait(&signals, &sig);
            monitor.stop();
        }};
        try {
            monitor.run();
        } catch (...) {
            pthread_kill(signal_thread.native_handle(), SIGTERM);
            throw;
        }
        pthread_kill(signal_thread.native_handle(), SIGTERM);
    } catch (std::exception const &ex) {
        std::cerr << "Ошибка во время выполнения: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
