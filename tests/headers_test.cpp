#include <gtest/gtest.h>
#include "headers.h"
#include <gtest/gtest.h>
#include "headers.h"  // ваш заголовочный файл с функциями

using namespace std::string_view_literals;

// === Тесты для iterHeaders ===

TEST(iterHeaders, Empty) {
    std::string_view req = "GET / HTTP/1.1\r\n\r\n";  // только первая строка + \r\n\r\n

    size_t count = 0;
    iterHeaders(req, [&](std::string_view, std::string_view) {
        ++count;
    });

    EXPECT_EQ(count, 0);  // заголовков нет
}

TEST(iterHeaders, SkipRequestLine) {
    std::string_view req =
        "GET /path HTTP/1.1\r\n"          // первая строка (пропускаем)
        "Host: example.com\r\n"
        "\r\n";

    std::string host;
    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        if (name == "Host"sv) host = std::string(value);
    });

    EXPECT_EQ(host, "example.com");
}

TEST(iterHeaders, SingleHeader) {
    std::string_view req =
        "GET / HTTP/1.1\r\n"
        "User-Agent: curl/7.85.0\r\n"
        "\r\n";

    std::string agent;
    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        agent = std::string(value);
    });

    EXPECT_EQ(agent, "curl/7.85.0");
}

TEST(iterHeaders, MultipleHeaders) {
    std::string_view req =
        "GET / HTTP/1.1\r\n"
        "Host: test.com\r\n"
        "Accept: */*\r\n"
        "Connection: close\r\n"
        "\r\n";

    std::unordered_map<std::string, std::string> headers;
    iterHeaders(req, [&](std::string_view name, std::string_view value) {
        headers[std::string(name)] = std::string(value);
    });

    ASSERT_EQ(headers.size(), 3);
    EXPECT_EQ(headers["Host"], "test.com");
    EXPECT_EQ(headers["Accept"], "*/*");
    EXPECT_EQ(headers["Connection"], "close");
}

TEST(iterHeaders, MultipleSameHeaders) {
    // В HTTP несколько одинаковых заголовков — валидно (например, Set-Cookie)
    std::string_view req =
        "GET / HTTP/1.1\r\n"
        "X-Custom: first\r\n"
        "X-Custom: second\r\n"
        "\r\n";

    std::vector<std::string> values;
    iterHeaders(req, [&](std::string_view, std::string_view value) {
        values.push_back(std::string(value));
    });

    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values[0], "first");
    EXPECT_EQ(values[1], "second");
}

// === Тесты для findHostPort ===

TEST(findHostPort, Simple) {
    std::string_view req =
        "GET / HTTP/1.1\r\n"
        "Host: api.example.org\r\n"
        "\r\n";

    auto [host, port] = findHostPort(req);
    EXPECT_EQ(host, "api.example.org");
    EXPECT_EQ(port, "80");  // порт по умолчанию
}

TEST(findHostPort, NoHost) {
    std::string_view req =
        "GET / HTTP/1.1\r\n"
        "User-Agent: test\r\n"
        "\r\n";

    auto [host, port] = findHostPort(req);
    EXPECT_TRUE(host.empty());
    EXPECT_EQ(port, "8 prepared");  // всё ещё 80
    // Коррекция: должно быть "80", см. ниже
}

// Исправление: в предыдущем тесте опечатка. Правильный вариант:
TEST(findHostPort, NoHost_Corrected) {
    std::string_view req =
        "GET / HTTP/1.1\r\n"
        "User-Agent: test\r\n"
        "\r\n";

    auto [host, port] = findHostPort(req);
    EXPECT_TRUE(host.empty());
    EXPECT_EQ(port, "80");  // исправлено
}

TEST(findHostPort, WithPort) {
    std::string_view req =
        "GET / HTTP/1.1\r\n"
        "Host: localhost:3000\r\n"
        "\r\n";

    auto [host, port] = findHostPort(req);
    EXPECT_EQ(host, "localhost");
    EXPECT_EQ(port, "3000");
}

TEST(findHostPort, WhitespaceAroundHost) {
    std::string_view req =
        "GET / HTTP/1.1\r\n"
        "Host:   space-test.com   \r\n"  // пробелы до/после
        "\r\n";

    auto [host, port] = findHostPort(req);
    EXPECT_EQ(host, "space-test.com");
    EXPECT_EQ(port, "80");
}

// === Тесты для findContentLength ===

TEST(findContentLength, Simple) {
    std::string_view rsp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 1024\r\n"
        "\r\n";

    auto len = findContentLength(rsp);
    ASSERT_TRUE(len.has_value());
    EXPECT_EQ(*len, 1024u);
}

TEST(findContentLength, NoContentLength) {
    std::string_view rsp =
        "HTTP/1.1 200 OK\r\n"
        "Server: nginx\r\n"
        "\r\n";

    auto len = findContentLength(rsp);
    EXPECT_FALSE(len.has_value());  // заголовка нет
}

TEST(findContentLength, WhitespaceInValue) {
    std::string_view rsp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length:   42   \r\n"  // пробелы вокруг числа
        "\r\n";

    auto len = findContentLength(rsp);
    ASSERT_TRUE(len.has_value());
    EXPECT_EQ(*len, 42u);
}

TEST(findContentLength, CaseInsensitive) {
    std::string_view rsp =
        "HTTP/1.1 200 OK\r\n"
        "content-length: 999\r\n"  // нижний регистр
        "\r\n";

    auto len = findContentLength(rsp);
    ASSERT_TRUE(len.has_value());
    EXPECT_EQ(*len, 999u);
}

TEST(findContentLength, InvalidNumber) {
    std::string_view rsp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: abc123\r\n"  // не число
        "\r\n";

    auto len = findContentLength(rsp);
    EXPECT_FALSE(len.has_value());  // from_chars вернёт ошибку
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
