#pragma once
#include <string_view>

/**
 * @brief Возвращает HTML главной страницы.
 *
 * Страница статическая: стили и скрипт встроены, данные подгружаются из /api/status.
 *
 * @return HTML-код страницы.
 */
[[nodiscard]] std::string_view index_page();
