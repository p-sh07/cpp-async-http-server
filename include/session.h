//
// Created by Pavel on 02.12.2025.
//

#pragma once
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <string_view>
#include <string>
#include <optional>
#include <iostream>

namespace http {
using boost::asio::awaitable;
using boost::asio::io_context;
using boost::asio::ip::tcp;
using boost::system::error_code;

constexpr std::string_view delimiter = "\r\n\r\n";

//======== Helper functions ==========
// awaitable<void> relayExactBody(tcp::socket& from, tcp::socket& to, size_t total, error_code& out_ec);
// awaitable<void> relayStreamedBody(tcp::socket& from, tcp::socket& to, error_code& out_ec); //-> TODO: for use when content-length is absent
awaitable<void> closeSocket(tcp::socket& sock, error_code& ec);

//======== Session coroutine =========
awaitable<void> session(tcp::socket client_socket, io_context& ioc);

}  // namespace http
