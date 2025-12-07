//
// Created by Pavel on 06.12.2025.
//
#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <fstream>
#include <system_error>
#include <cstdlib>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;
const fs::path TEMP_DIR = fs::current_path() / "tmp";

//========== With Python backend emulation ==========
const std::string PROXY_SERVER_PORT = "5555";
const std::string BACKEND_SERVER_PORT = "4444";
const std::string PROXY_ADDR = "127.0.0.1:" + PROXY_SERVER_PORT;
const std::string BACKEND_ADDR = "127.0.0.1:" + BACKEND_SERVER_PORT;

void start_proxy_server_app() {
    std::string cmd = fs::current_path().string() + "/AsyncHttpProxy " + PROXY_SERVER_PORT;
    std::println("{}\n", cmd);
    std::system(cmd.c_str());
}


void start_backend_server() {
    std::string cmd = std::format(R"(python3 -c '
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("127.0.0.1", {}))
s.listen(1)
conn, _ = s.accept()
req = conn.recv(1024)

response = (
    b"HTTP/1.1 200 OK\r\n"
    b"Content-Type: text/html\r\n"
    b"Content-Length: 12\r\n"
    b"Connection: close\r\n"
    b"\r\n"
    b"Hello World!\n"
)

conn.sendall(response)
conn.shutdown(socket.SHUT_WR)

conn.close()
s.close()
')", BACKEND_SERVER_PORT);


    std::println("{}\n", cmd);
    std::system(cmd.c_str());
}

int run_wget_via_proxy(const std::string& url, const fs::path& output_file) {
    std::string cmd = "wget  --timeout=2 --tries=1 -S -e use_proxy=yes -e http_proxy=" + PROXY_ADDR +
                       " -O \"" + output_file.string() + "\" \"" + url + "\"";
    std::println("{}\n", cmd);
    return std::system(cmd.c_str());
}


// bool is_port_available(int port) {
//     std::string cmd = "lsof -i:" + std::to_string(port) + " > /dev/null 2>&1";
//     std::println("{}\n", cmd);
//     return std::system(cmd.c_str()) != 0;
// }

void kill_process_on_port(std::string port) {
    std::string cmd = "kill -9 $( lsof -i:" + port + " -t )";

    std::println("{}\n", cmd);
    std::system(cmd.c_str());
}

void cleanup() {
    kill_process_on_port(BACKEND_SERVER_PORT);
    kill_process_on_port(PROXY_SERVER_PORT);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    fs::remove_all(TEMP_DIR);
    fs::create_directories(TEMP_DIR);
}

TEST(ProxyTest, SuccessfulRequest) {
    std::println("->launching cleanup\n");
    cleanup();

    std::println("->starting server\n");

    //start backend
    std::jthread backend(start_backend_server);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::jthread proxy(start_proxy_server_app);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    //send request
    const std::string url = "http://" + BACKEND_ADDR + "/test";
    const fs::path output_file = TEMP_DIR / "output.html";

    int exit_code = run_wget_via_proxy(url, output_file);

    EXPECT_EQ(exit_code, 0) << "wget failed with exit code " << exit_code;

    //check response
    ASSERT_TRUE(fs::exists(output_file));
    ASSERT_GT(fs::file_size(output_file), 0);

    std::ifstream file(output_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("Hello World!"), std::string::npos);
}
