//
// Created by Pavel on 02.12.2025.
//

#pragma once
#include "session.h"

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <string_view>
#include <iostream>

#include "session.h"

namespace http {
using boost::asio::io_context;
using boost::asio::awaitable;
using boost::asio::use_awaitable;
using boost::system::error_code;
using boost::asio::ip::tcp;

class Server
{
public:
    Server(io_context &ctx, short port);

private:
    void do_accept();

    io_context & io_context_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
};
}