#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <cstdlib>
#include <calcLib.h>


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