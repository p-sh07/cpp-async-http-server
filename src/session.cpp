#include "session.h"
#include "headers.h"

#include <iostream>
#include <print>

namespace http {

using boost::asio::awaitable;
using boost::asio::async_write;
using boost::asio::use_awaitable;
using boost::system::error_code;
using boost::asio::dynamic_buffer;
using boost::asio::transfer_at_least;
using boost::beast::buffers_prefix;
using boost::asio::mutable_buffer;

static constexpr uint8_t SERVER_READ_CHUNK = 8192;

awaitable<void> session(tcp::socket client_socket, io_context& ioc) {
    auto server_socket = std::make_shared<tcp::socket>(ioc);
    error_code ec;

    try {
        //Read client headers until delim
        std::string read_data;
        auto read_buffer = dynamic_buffer(read_data);

        auto n_read = co_await async_read_until(
            client_socket, read_buffer, http::delimiter, use_awaitable
        );

        std::string_view client_request{static_cast<const char*>(read_buffer.data().data()), n_read};

        //Parse host & port
        auto [host, port] = findHostPort(client_request);
        if (host.empty()) {
            std::println("Error: Could not find Host in request");
            co_return;
        }

        //Connect to target host
        tcp::resolver resolver(ioc);
        auto endpoints = co_await resolver.async_resolve(host, port, use_awaitable);
        co_await server_socket->async_connect(*endpoints.begin(), use_awaitable);

        std::println("connecting to host: {}", host);

        //Forward response
        std::string server_response_data;
        auto server_responce_buff = dynamic_buffer(read_data);

        //Read response headers which are termjinated by blank line
        n_read = co_await async_read_until(*server_socket, server_responce_buff, delimiter, use_awaitable);

        std::string_view server_response{static_cast<const char*>(server_responce_buff.data().data()), n_read};

        co_await async_write(client_socket, server_responce_buff, use_awaitable);

        //Relay body
        if (auto content_length = findContentLength(server_response))
        {
            size_t n_bytes_total = *content_length;
            while (n_bytes_total > 0) {
                std::array<char, SERVER_READ_CHUNK> read_buff_data;

                while (n_bytes_total > 0) {
                    size_t n_bytes_read = co_await server_socket->async_read_some(boost::asio::buffer(read_buff_data), use_awaitable);
                    co_await async_write(client_socket, boost::asio::buffer(read_buff_data, n_bytes_read), use_awaitable);
                    n_bytes_total -= n_bytes_read;
                }
            }
        }
        std::println("transferred to client successfully");

    } catch (const std::exception& e) {
        std::cerr << "Session error: " << e.what() << std::endl;
    }

    co_await closeSocket(client_socket, ec);
    co_await closeSocket(*server_socket, ec);
}

//======== Helper functions ==========
awaitable<void> closeSocket(tcp::socket& sock, boost::system::error_code& ec) {
    if (sock.is_open()) {
        sock.shutdown(tcp::socket::shutdown_both, ec);
        sock.close(ec);
    }
    co_return;
}

} //namespace http