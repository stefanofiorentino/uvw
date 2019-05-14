#include <uvw.hpp>
#include <cassert>
#include <iostream>
#include <memory>
#include <chrono>
#include <cstring>


void listen(uvw::Loop &loop) {
    std::shared_ptr<uvw::TCPHandle> server = loop.resource<uvw::TCPHandle>();

    server->on<uvw::WriteEvent>([](const uvw::WriteEvent &, uvw::TCPHandle &) {
        std::cout << "server write" << std::endl;
    });

    server->on<uvw::CloseEvent>([](const uvw::CloseEvent &, uvw::TCPHandle &) {
        std::cout << "server close" << std::endl;
    });

    server->on<uvw::EndEvent>([](const uvw::EndEvent &, uvw::TCPHandle &) {
        std::cout << "server end" << std::endl;
    });

    server->on<uvw::ErrorEvent>([](const uvw::ErrorEvent &, uvw::TCPHandle &) {
        std::cout << "server error" << std::endl;
    });

    server->on<uvw::ListenEvent>([](const uvw::ListenEvent &, uvw::TCPHandle &handle) {
        std::shared_ptr<uvw::TCPHandle> socket = handle.loop().resource<uvw::TCPHandle>();

        socket->on<uvw::WriteEvent>([](const uvw::WriteEvent &, uvw::TCPHandle &socket_hdl) {
            std::cout << "socket write" << std::endl;
            socket_hdl.close();
        });

        socket->on<uvw::DataEvent>([](const uvw::DataEvent &event, uvw::TCPHandle &socket) {
            std::cout.write(event.data.get(), event.length) << std::endl;
            std::cout << "data length: " << event.length << std::endl;

            auto data_str = std::string{event.data.get()};
            const auto index = data_str.find("\r\n\r\n");
            if (index != std::string::npos)
            {
                auto header = std::string{data_str.cbegin(), data_str.cbegin()+index};
                std::cout << "Header: " << header << std::endl;
                auto body = std::string{data_str.cbegin()+index+4, data_str.cend()};
                std::cout << "Body: " << body << std::endl;
                if (socket.writable())
                {
                    char result[]= R"(HTTP/1.1 200 OK

{
    "result": true
}
)";
                    auto result_len = std::strlen(result);
                    socket.write(result, result_len);
                }
            }
        });

        socket->on<uvw::CloseEvent>([](const uvw::CloseEvent &, uvw::TCPHandle &) {
            std::cout << "socket close" << std::endl;
        });

        socket->on<uvw::EndEvent>([](const uvw::EndEvent &, uvw::TCPHandle &) {
            std::cout << "socket end" << std::endl;
        });

        socket->on<uvw::ErrorEvent>([](const uvw::ErrorEvent &, uvw::TCPHandle &) {
            std::cout << "socket error" << std::endl;
        });

        handle.accept(*socket);
        socket->read();
    });

    server->bind("127.0.0.1", 4242);
    server->listen();
}


void g() {
    auto loop = uvw::Loop::getDefault();
    listen(*loop);
    loop->run();
    loop = nullptr;
}

int main() {
    g();
}
