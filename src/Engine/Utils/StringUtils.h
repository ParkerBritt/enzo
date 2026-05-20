#pragma once
#include <cctype>
#include <string_view>

namespace enzo::utils {

/**
 * @brief Returns a view of the input with leading and trailing whitespace removed.
 */
inline std::string_view trim(std::string_view text)
{
    size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) ++start;
    size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    return text.substr(start, end - start);
}

} // namespace enzo::utils
