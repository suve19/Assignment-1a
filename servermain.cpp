#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <cstdlib>
#include <calcLib.h>
#include <poll.h>



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

class MathServer {
private:
    std::string ip_address_;
    int port_number_;

    /**
     * Split a string into a vector of substrings
     * @param input Input string to split
     * @param delimiter Character to split on
     * @return Vector of substrings
     */
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

    /**
     * Parse IP and port from command line argument
     * @param arg Command line argument string
     */
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

public:
    MathServer(const std::string& arg) {
        parseIPAndPort(arg);
    }
};


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