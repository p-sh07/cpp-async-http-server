#include <gtest/gtest.h>
#include "headers.h"

using namespace std::string_view_literals;

// === iterHeaders tests ===
TEST(iterHeaders, Empty) {
    std::string_view req = "GET / HTTP/1.1\r\n\r\n";

    size_t count = 0;
    iterHeaders(req, [&](std::string_view, std::string_view) {
        ++count;
    });

    EXPECT_EQ(count, 0);
}

TEST(iterHeaders, SkipRequestLine) {
    std::string_view req =
        "GET /path HTTP/1.1\r\n"
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

// === findHostPort tests ===
TEST(findHostPort, Simple) {
    std::string_view req =
        "GET / HTTP/1.1\r\n"
        "Host: api.example.org\r\n"
        "\r\n";

    auto [host, port] = findHostPort(req);
    EXPECT_EQ(host, "api.example.org");
    EXPECT_EQ(port, "80");
}

TEST(findHostPort, NoHost) {
    std::string_view req =
        "GET / HTTP/1.1\r\n"
        "User-Agent: test\r\n"
        "\r\n";

    auto [host, port] = findHostPort(req);
    EXPECT_TRUE(host.empty());
    EXPECT_EQ(port, "80");
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
        "Host:   space-test.com   \r\n"
        "\r\n";

    auto [host, port] = findHostPort(req);
    EXPECT_EQ(host, "space-test.com");
    EXPECT_EQ(port, "80");
}

// === findContentLength tests ===
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
    EXPECT_FALSE(len.has_value());
}

TEST(findContentLength, WhitespaceInValue) {
    std::string_view rsp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length:   42   \r\n"
        "\r\n";

    auto len = findContentLength(rsp);
    ASSERT_TRUE(len.has_value());
    EXPECT_EQ(*len, 42u);
}

TEST(findContentLength, CaseInsensitive) {
    std::string_view rsp =
        "HTTP/1.1 200 OK\r\n"
        "conTeNt-LeNGth: 999\r\n"
        "\r\n";

    auto len = findContentLength(rsp);
    ASSERT_TRUE(len.has_value());
    EXPECT_EQ(*len, 999u);
}

TEST(findContentLength, OverMaxContentLen) {
    auto over_max = 12 * 1024 * 1024;
    std::string rsp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: " + std::to_string(over_max) + "\r\n"
        "\r\n";

    auto len = findContentLength(rsp);
    ASSERT_TRUE(len.has_value());
    EXPECT_EQ(*len, MAX_CONTENT_LENGTH);
}

TEST(findContentLength, InvalidNumber) {
    std::string_view rsp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: abc123\r\n"
        "\r\n";

    auto len = findContentLength(rsp);
    EXPECT_FALSE(len.has_value());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
