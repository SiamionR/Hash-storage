#define ASIO_STANDALONE
#include <asio.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

using umSS = std::unordered_map<std::string, std::string>;

enum class Command { INSERT, DELETE, GET, UNDEFINED };

// Command Handlers
auto handle_insert(std::istream& In, umSS& hash)
{
    std::string key, value;
    std::ostringstream output;
    In >> key >> value;
    hash.insert(std::make_pair(key, value));
    output << "INSERT command competed.\n\tPair [" << key << ", " << value
           << "] inserted into container!\n";
    return std::string{output.str()};
}

auto handle_delete(std::istream& In, umSS& hash)
{
    std::string key;
    std::ostringstream output;
    In >> key;
    hash.erase(key);
    output << "DELETE command completed.\n\t" << key
           << " removed from the container!\n";
    return std::string{output.str()};
}

auto handle_get(std::istream& In, umSS& hash)
{
    std::string key;
    std::ostringstream output;
    In >> key;
    auto find = hash.find(key);
    if (find != hash.end()) {
        output << "GET command completed.\n\tKEY VALUE: " << find->first
               << ";\n\tELEMENT VALUE: " << find->second << ".\n";
    } else {
        output << "GET command completed.\n\tElement with KEY VALUE " << key
               << " NOT FOUND\n";
    }
    return std::string{output.str()};
}

// Mapping a string to a command
Command classify_command(std::string_view cmdstring)
{
    if (cmdstring == "INSERT") {
        return Command::INSERT;
    } else if (cmdstring == "DELETE") {
        return Command::DELETE;
    } else if (cmdstring == "GET") {
        return Command::GET;
    } else {
        return Command::UNDEFINED;
    }
}

// Identifying the command
auto process_request(std::string cmd_line, umSS& hash)
{
    std::string cmd_string, result;
    std::istringstream In{cmd_line};

    In >> cmd_string;

    Command this_cmd = classify_command(cmd_string);

    switch (this_cmd) {
    case Command::INSERT:
        result = handle_insert(In, hash);
        break;
    case Command::DELETE:
        result = handle_delete(In, hash);
        break;
    case Command::GET:
        result = handle_get(In, hash);
        break;
    case Command::UNDEFINED:
        result = "Invalid command. Try INSERT, DELETE or GET.\n";
        break;
    }
    return result;
}

class Session : public std::enable_shared_from_this<Session>
{
  public:
    Session(asio::ip::tcp::socket socket) : tcp_socket(std::move(socket)) {}

    void start() { read(); }

  private:
    void read()
    {
        auto self{shared_from_this()};

        tcp_socket.async_read_some(
          asio::buffer(data, data.size()),
          [this, self](const std::error_code ec, const std::size_t length) {
              if (!ec) {
                  auto server_request{std::string(data.data(), length)};
                  auto response = process_request(server_request, hash_data);

                  std::cout << "Server:\n*\t" << server_request << " -> "
                            << "Request completed successfully!\n\n";

                  write(response);
              }
          });
    }

    void write(std::string_view response)
    {
        auto self{shared_from_this()};

        tcp_socket.async_write_some(
          asio::buffer(response.data(), response.length()),
          [this, self](const std::error_code ec, const std::size_t) {
              if (!ec) {
                  read();
              }
          });
    }

    umSS hash_data;
    std::array<char, 1024> data;
    asio::ip::tcp::socket tcp_socket;
};

class Server
{
  public:
    Server(asio::io_context& context, const short port)
        : tcp_acceptor(context,
                       asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
          tcp_socket(context)
    {
        std::cout << "server running on port " << port << '\n';

        accept();
    }

  private:
    void accept()
    {
        tcp_acceptor.async_accept(tcp_socket, [this](std::error_code ec) {
            if (!ec) {
                std::make_shared<Session>(std::move(tcp_socket))->start();
            }

            accept();
        });
    }

    asio::ip::tcp::acceptor tcp_acceptor;
    asio::ip::tcp::socket tcp_socket;
};

void run_server(const short port)
{
    try {
        asio::io_context context;

        Server srv(context, port);

        context.run();
    } catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
    }
}

int main()
{
    run_server(13422);
}
