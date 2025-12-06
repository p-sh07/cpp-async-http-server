#include "headers.h"

#include <algorithm>
#include <charconv>
#include <print>
#include <ranges>
#include <string_view>

using namespace std::string_view_literals;
namespace rn = std::ranges;
namespace vs = std::views;

void iterHeaders(std::string_view req, Callback&& callback) {
    auto headers_start = req.find("\r\n"sv);
    if (headers_start == std::string_view::npos) {
        return;
    }
    req.remove_prefix(headers_start + 2);

    //Split into header lines
    auto lines = vs::split(req, "\r\n"sv)
       | vs::filter([](const auto line) { return !line.empty(); })
       | std::views::transform([](const auto& line) {
           return std::string_view(line);
    });

    //process headers
    for (const std::string_view line : lines) {
        auto colon_pos = line.find(':');
        if (colon_pos != std::string_view::npos) {
            std::string_view name = line.substr(0, colon_pos);
            std::string_view value = line.substr(colon_pos + 1);

            value = value.substr(value.find_first_not_of(" \t"sv));
            value.remove_suffix(value.size() - value.find_last_not_of(" \t"sv) - 1);
            callback(name, value);
        }
    }
}

std::pair<std::string, std::string> findHostPort(std::string_view req) {
    std::string host;
    std::string port = "80";

    //TODO: Не до конца понял, как тут преобразовать до вызова коллбэка?
    //Или имелось в ввиду внутри iterheaders преобразовать к нижнему регистру?
    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        // convert name to all lower letters
        std::string lower_name = name
            | vs::transform([](unsigned char c) { return std::tolower(c); })
            | rn::to<std::string>();

        if (lower_name == "host") {
            auto colon_pos = value.find(':');
            if (colon_pos != std::string_view::npos) {
                host = std::string(value.substr(0, colon_pos));
                port = std::string(value.substr(colon_pos + 1));
            } else {
                host = std::string(value);
            }
        }
    });

    return {host, port};
}

std::optional<size_t> findContentLength(std::string_view rsp) {
    std::optional<size_t> result{};

    iterHeaders(rsp, [&](std::string_view name, std::string_view value) {
        std::string lower_name = name
            | vs::transform([](unsigned char c) { return std::tolower(c); })
            | rn::to<std::string>();

        if (lower_name == "content-length") {
            size_t length = 0u;
            size_t lowest = 0u;
            auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), length);
            if (ec == std::errc{}) {
                result = std::clamp(length, lowest, MAX_CONTENT_LENGTH);
            }
        }
    });
    return result;
}