#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <cstdlib>
#include <calcLib.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>    // For close()
#include <cstring>     // For strlen()
#include <iomanip>     // For setprecision()
#include <arpa/inet.h>

class MathServer {
private:
    std::string ip_address_;
    int port_number_;
    int master_socket_;

    // Splits a string into a vector of substrings
    std::vector<std::string> split(const std::string& input, char delimiter) {
        std::vector<std::string> result;
        std::stringstream ss(input);
        std::string item;

        while (std::getline(ss, item, delimiter)) {
            if (!item.empty()) {
                result.push_back(item);
            }
        }

        return result;
    }

    // Parses IP and port from the command-line argument
    void parseIPAndPort(const std::string& arg) {
        auto parts = split(arg, ':');
        if (parts.size() == 1) {
            ip_address_ = parts[0];
            port_number_ = 0;
        } else if (parts.size() == 2) {
            ip_address_ = parts[0];
            try {
                port_number_ = std::stoi(parts[1]);
            } catch (...) {
                throw std::runtime_error("Invalid port number");
            }
        } else {
            throw std::runtime_error("Invalid IP:PORT format");
        }
    }

    // Initializes the server socket
    void initializeSocket() {
        struct addrinfo hints{}, *res;
        hints.ai_family = AF_UNSPEC;  // Support both IPv4 and IPv6
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        std::string port_str = std::to_string(port_number_);
        if (getaddrinfo(ip_address_.c_str(), port_str.c_str(), &hints, &res) != 0) {
            throw std::runtime_error("Failed to resolve address");
        }

        master_socket_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (master_socket_ == -1) {
            freeaddrinfo(res);
            throw std::runtime_error("Failed to create socket");
        }

        if (bind(master_socket_, res->ai_addr, res->ai_addrlen) == -1) {
            close(master_socket_);
            freeaddrinfo(res);
            throw std::runtime_error("Failed to bind socket");
        }

        freeaddrinfo(res);

        if (listen(master_socket_, 5) == -1) {
            close(master_socket_);
            throw std::runtime_error("Failed to listen on socket");
        }
    }

public:
    MathServer(const std::string& arg) {
        parseIPAndPort(arg);
        initializeSocket();
    }

    ~MathServer() {
        if (master_socket_ != -1) close(master_socket_);
    }

    // Starts the server
    void start() {
        while (true) {
            struct sockaddr_storage client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int comm_socket_ = accept(master_socket_, (struct sockaddr*)&client_addr, &addr_len);

            if (comm_socket_ == -1) continue;

            char client_ip[INET6_ADDRSTRLEN];
            if (client_addr.ss_family == AF_INET) {
                inet_ntop(AF_INET, &(((struct sockaddr_in*)&client_addr)->sin_addr), client_ip, INET_ADDRSTRLEN);
            } else if (client_addr.ss_family == AF_INET6) {
                inet_ntop(AF_INET6, &(((struct sockaddr_in6*)&client_addr)->sin6_addr), client_ip, INET6_ADDRSTRLEN);
            }

            std::cout << "Connection from: " << client_ip << std::endl;

            try {
                handleClient(comm_socket_);
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }

            close(comm_socket_);
        }
    }

    // Handles communication with a single client
    void handleClient(int comm_socket_) {
        const int MAX_BUFFER_SIZE = 1024;
        char buffer[MAX_BUFFER_SIZE] = {0};
        struct pollfd poll_fds[1];
        poll_fds[0].fd = comm_socket_;
        poll_fds[0].events = POLLIN;

        const char* protocol_msg = "TEXT TCP 1.0\n\n";
        send(comm_socket_, protocol_msg, strlen(protocol_msg), 0);

        if (poll(poll_fds, 1, 5000) <= 0) {
            send(comm_socket_, "ERROR TO\n", 9, 0);
            throw std::runtime_error("Timeout waiting for protocol response");
        }

        recv(comm_socket_, buffer, sizeof(buffer), 0);
        if (std::string(buffer).substr(0, 2) != "OK") {
            send(comm_socket_, "ERROR\n", 6, 0);
            throw std::runtime_error("Protocol rejected by client");
        }

        std::string problem = generateProblem() + "\n";
        send(comm_socket_, problem.c_str(), problem.length(), 0);

        if (poll(poll_fds, 1, 5000) <= 0) {
            send(comm_socket_, "ERROR TO\n", 9, 0);
            throw std::runtime_error("Timeout waiting for solution");
        }

        recv(comm_socket_, buffer, sizeof(buffer), 0);
        auto parts = split(problem, ' ');
        std::string correct_solution = computeResult(parts[0], std::stod(parts[1]), std::stod(parts[2]));

        if (std::string(buffer) == correct_solution) {
            send(comm_socket_, "OK\n", 3, 0);
        } else {
            send(comm_socket_, "ERROR\n", 6, 0);
        }
    }

    // Generate a random math problem
    std::string generateProblem() {
        char* op = randomType();
        std::ostringstream problem;
        problem << std::fixed << std::setprecision(8);

        if (op[0] == 'f') {
            double a = randomFloat();
            double b = randomFloat();
            problem << op << " " << a << " " << b;
        } else {
            int a = randomInt();
            int b = randomInt();
            problem << op << " " << a << " " << b;
        }

        return problem.str();
    }

    // Computes the result of mathematical operations
    std::string computeResult(const std::string& operation, double a, double b) {
        std::ostringstream result;
        result << std::fixed << std::setprecision(8);

        if (operation == "fadd") return result << (a + b) << "\n", result.str();
        if (operation == "fsub") return result << (a - b) << "\n", result.str();
        if (operation == "fmul") return result << (a * b) << "\n", result.str();
        if (operation == "fdiv") {
            if (b == 0) throw std::runtime_error("Division by zero");
            return result << (a / b) << "\n", result.str();
        }

        int ia = static_cast<int>(a);
        int ib = static_cast<int>(b);
        if (operation == "add") return result << (ia + ib) << "\n", result.str();
        if (operation == "sub") return result << (ia - ib) << "\n", result.str();
        if (operation == "mul") return result << (ia * ib) << "\n", result.str();
        if (operation == "div") {
            if (ib == 0) throw std::runtime_error("Division by zero");
            return result << (ia / ib) << "\n", result.str();
        }

        throw std::runtime_error("Invalid operation");
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <IP:PORT>" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        MathServer server(argv[1]);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
