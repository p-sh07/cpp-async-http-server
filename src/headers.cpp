#include "headers.h"

#include <charconv>
#include <ranges>
#include <string_view>

using namespace std::string_view_literals;

using Callback = std::function<void(std::string_view, std::string_view)>;

void iterHeaders(std::string_view req, Callback&& callback) {
    auto headers_start = req.find("\r\n"sv);
    if (headers_start == std::string_view::npos) return;
    headers_start += 2;

    size_t pos = headers_start;
    while (pos < req.size()) {
        auto line_end = req.find("\r\n"sv, pos);
        if (line_end == std::string_view::npos || line_end - pos == 0) break;

        std::string_view line = req.substr(pos, line_end - pos);

        auto colon_pos = line.find(':');
        if (colon_pos != std::string_view::npos) {
            std::string_view name = line.substr(0, colon_pos);
            std::string_view value = line.substr(colon_pos + 1);

            value = value.substr(value.find_first_not_of(" \t"sv));

            callback(name, value);
        }

        pos = line_end + 2;
    }
}

std::pair<std::string, std::string> findHostPort(std::string_view req) {
    std::string host;
    std::string port = "80";

    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        if (name == "Host"sv || name == "host"sv) {

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
    iterHeaders(rsp, [&](std::string_view name, std::string_view value) {
        if (name == "Content-Length"sv || name == "content-length"sv) {
            size_t length;
            auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), length);
            if (ec == std::errc{}) {
                return std::optional{length};
            }
        }
        return std::optional<size_t>{};
    });
    return std::nullopt;
}

std::optional<std::string> parse_host_header(std::string_view request_headers) {
    for (auto line : std::views::split(request_headers, '\n')) {
        std::string_view sv(line.begin(), line.end());
        if (sv.starts_with("Host:"sv) || sv.starts_with("host:"sv)) {
            sv = sv.substr(5);
            sv = sv.substr(sv.find_first_not_of(" \t"sv));
            return std::string(sv);
        }
    }
    return std::nullopt;
}