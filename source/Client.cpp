#define ASIO_STANDALONE
#include <array>
#include <asio.hpp>
#include <iostream>
#include <string>
#include <string_view>

void run_client(std::string_view host, const short port)
{
    try {
        asio::io_context context;
        asio::ip::tcp::socket tcp_socket(context);
        asio::ip::tcp::resolver resolver(context);
        asio::connect(tcp_socket,
                      resolver.resolve({host.data(), std::to_string(port)}));
        std::cout
          << host << " connected to port " << port << '\n'
          << "Available commands:\t INSERT key value\t GET key\t DELETE key.\n";

        while (true) {
            std::string request;
            std::cout << "Please, enter your command (C-d to terminate):\n";

            if (std::getline(std::cin, request).fail())
                break;
            std::cout << '\n';

            tcp_socket.write_some(asio::buffer(request, request.length()));

            std::array<char, 1024> reply;
            auto reply_length =
              tcp_socket.read_some(asio::buffer(reply, reply.size()));

            std::cout << "Reply from the server:\n->\t";
            std::cout.write(reply.data(), reply_length);
            std::cout << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << '\n';
    }
}

int main()
{
    run_client("localhost", 13422);
}
