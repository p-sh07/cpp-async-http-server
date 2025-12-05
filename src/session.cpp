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
        n_read = co_await async_read_until(*server_socket, write_buffer, delimiter, use_awaitable);

        std::string_view server_response{static_cast<const char*>(write_buffer.data().data()), n_read};

        co_await async_write(client_socket, write_buffer, use_awaitable);

        //Relay body
        if (auto content_length = findContentLength(server_response))
        {
            size_t n_bytes = *content_length;
            std::vector<char> buff_data;
            auto relay_buffer = dynamic_buffer(buff_data);

            while (n_bytes > 0) {
                //reserve n bytes
                auto mutable_buf = relay_buffer.prepare(n_bytes);

                size_t n = co_await server_socket->async_read_some(mutable_buf, use_awaitable);

                relay_buffer.commit(n);

                co_await async_write(client_socket, relay_buffer, use_awaitable);
                n_bytes -= n;
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


// using shared_from_this version
// Created by Pavel on 02.12.2025.
//
//
// #include "session.h"
//
// namespace http {
// Session::Session(tcp::socket client_socket)
//     : client_socket_(std::move(client_socket))
// {}
//
// awaitable<void> Session::start() {
//     try {
//         std::string read_data;
//         auto read_buffer = dynamic_buffer(read_data);
//         co_await async_read_until(client_socket_, read_buffer, delimiter, use_awaitable);
//
//         //test write
//         std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello World!";
//
//         auto write_buffer = dynamic_buffer(response);
//         co_await async_write(client_socket_, write_buffer, use_awaitable);
//
//     } catch (const std::exception &e) {
//         std::cerr << "Connection error: " << e.what() << std::endl;
//     }
// }
// }
