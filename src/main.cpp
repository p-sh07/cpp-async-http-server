#include "headers.h"

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <iostream>
#include <string_view>

#include "http_server.h"

int main(int argc, char *argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: proxy_server";
            std::cerr << " <listen_port>\n";
            return 1;
        }
        http::io_context io_ctx(1);
        auto port = std::atoi(argv[1]);

        http::Server server(io_ctx, port);
        std::println("==== Server started on port: {} ====", port);

        io_ctx.run();

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
