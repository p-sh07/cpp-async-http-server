//
// Created by Pavel on 02.12.2025.
//

#include "http_server.h"

namespace http {
using boost::asio::detached;

Server::Server(io_context &ctx, short port)
    : io_context_(ctx), acceptor_(ctx, tcp::endpoint(tcp::v4(), port)), socket_(ctx) {
    do_accept();
}

void Server::do_accept() {
    acceptor_.async_accept(socket_,
        [this](error_code ec) {
            if(!ec) {
                co_spawn(io_context_, session(std::move(socket_), io_context_), detached);
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }
            do_accept();
    });
}
} // namespace http
