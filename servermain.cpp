#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>

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
