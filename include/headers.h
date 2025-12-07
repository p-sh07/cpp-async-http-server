#pragma once

#include <string>
#include <functional>
#include <optional>
#include <ranges>

using Callback = std::function<void(std::string_view, std::string_view)>;
static constexpr size_t MAX_CONTENT_LENGTH = 10 * 1024 * 1024; // 10 mb max

void iterHeaders(std::string_view req, Callback&& callback);
std::pair<std::string, std::string> findHostPort(std::string_view req);
std::optional<size_t> findContentLength(std::string_view rsp);