cmake_minimum_required(VERSION 3.25.1)
project(service-monitor
        VERSION 0.11.0.0
        LANGUAGES CXX
        HOMEPAGE_URL "https://github.com/khromenokroman/service-monitor")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Собирать тесты" ON)

find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(sdbus-c++ REQUIRED)
find_package(Threads REQUIRED)

include(GNUInstallDirs)

set(SERVICE_MONITOR_CONFIG "/etc/${PROJECT_NAME}/cfg.json")

configure_file(Version.in Version @ONLY)
configure_file(systemd/${PROJECT_NAME}.service.in ${PROJECT_NAME}.service @ONLY)

add_library(${PROJECT_NAME}-core STATIC
        src/config.cpp
        src/index_page.cpp
        src/service_monitor.cpp
        src/system_info.cpp
        src/systemd_client.cpp
        src/unit_status.cpp
)
target_include_directories(${PROJECT_NAME}-core PUBLIC src)
target_include_directories(${PROJECT_NAME}-core SYSTEM PUBLIC third_party/httplib)
target_link_libraries(${PROJECT_NAME}-core PUBLIC fmt::fmt-header-only nlohmann_json::nlohmann_json SDBusCpp::sdbus-c++ Threads::Threads)

add_executable(${PROJECT_NAME}
        src/main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}-core)

if (BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_executable(${PROJECT_NAME}-tests
            tests/config_test.cpp
            tests/unit_status_test.cpp
            tests/system_info_test.cpp
    )
    target_link_libraries(${PROJECT_NAME}-tests PRIVATE ${PROJECT_NAME}-core GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(${PROJECT_NAME}-tests)
endif ()

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.service
        DESTINATION lib/systemd/system
)
install(FILES configuration/cfg.json
        DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/${PROJECT_NAME}
)
install(FILES debian/copyright
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "khromenokroman")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "khromenokroman <roma55592@yandex.ru>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Web page with systemd services state and system resources")
set(CPACK_PACKAGE_DESCRIPTION "HTTP server that shows the state of systemd units (via D-Bus)\nlisted in a JSON config, grouped by the user, together with CPU,\nmemory, swap, hugepages, NUMA and disk usage.")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")

set(CPACK_GENERATOR "DEB")
set(CPACK_STRIP_FILES ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION "admin")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "systemd, dbus, libsdbus-c++2")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_SOURCE_DIR}/debian/postinst"
        "${CMAKE_SOURCE_DIR}/debian/prerm"
        "${CMAKE_SOURCE_DIR}/debian/postrm"
        "${CMAKE_SOURCE_DIR}/debian/conffiles"
)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
set(CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

include(CPack)
