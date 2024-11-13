#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <calcLib.h>
#include <pthread.h>
#include <chrono>
#include <cmath>  // Include cmath for fabs

using namespace std;

vector<string> split(const string& s, const string& delimiter);
int initializeSocket(const string& ip, int port, int& ipstatus);
char* calculateResult(const string& operation, double a, double b);
string generateCalculationString();

// Utility function to trim leading and trailing spaces and newlines
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \n\r\t");
    size_t last = str.find_last_not_of(" \n\r\t");
    return (first == string::npos || last == string::npos) ? "" : str.substr(first, (last - first + 1));
}

// Function to compare floating-point values with tolerance
bool areFloatsEqual(double a, double b, double tolerance = 0.0001) {
    return fabs(a - b) < tolerance;
}

char buffer[1024];
int master_socketfd = 0, comm_socketfd = 0, sent_recv_bytes;
fd_set readfds;

const char errorMessage[] = "ERROR\n";
const char timeoutMessage[] = "ERROR TO\n";
const char initialMessage[] = "TEXT TCP 1.0\n";
const char okMessage[] = "OK\n";

int main(int argc, char *argv[]) {
    initCalcLib();
    
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <IP:Port>" << endl;
        return 1;
    }

    vector<string> splitResult = split(argv[1], ":");
    string ipAddress = splitResult[0];
    int portNumber = stoi(splitResult[1]);
    int ipstatus = 0;

    master_socketfd = initializeSocket(ipAddress, portNumber, ipstatus);
    FD_ZERO(&readfds);
    FD_SET(master_socketfd, &readfds);

    while (true) {
        if (select(master_socketfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            exit(1);
        }

        if (FD_ISSET(master_socketfd, &readfds)) {
            struct sockaddr_storage client_addr;
            socklen_t addrlen = sizeof(client_addr);
            comm_socketfd = accept(master_socketfd, (struct sockaddr*)&client_addr, &addrlen);
            if (comm_socketfd < 0) {
                perror("Accept error");
                exit(1);
            }

            struct timeval timeout = {5, 0};
            if (setsockopt(comm_socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
                perror("Setsockopt error");
                exit(1);
            }

            sent_recv_bytes = send(comm_socketfd, initialMessage, strlen(initialMessage), 0);
            if (sent_recv_bytes < 0) {
                perror("Error sending initial message");
                close(comm_socketfd);
                continue;
            }

            memset(buffer, 0, sizeof(buffer));
            sent_recv_bytes = recv(comm_socketfd, buffer, sizeof(buffer), 0);
            if (sent_recv_bytes <= 0 || string(buffer).substr(0, sent_recv_bytes - 1) != "OK") {
                send(comm_socketfd, errorMessage, sizeof(errorMessage), 0);
                close(comm_socketfd);
                continue;
            }

            string calculationString = generateCalculationString();
            vector<string> calculationParts = split(calculationString, " ");
            double a = stod(calculationParts[1]);
            double b = stod(calculationParts[2]);
            char* result = calculateResult(calculationParts[0], a, b);

            send(comm_socketfd, (calculationString + "\n").c_str(), calculationString.size() + 1, 0);
            memset(buffer, 0, sizeof(buffer));
            sent_recv_bytes = recv(comm_socketfd, buffer, sizeof(buffer), 0);

            string clientResult(buffer, sent_recv_bytes - 1);
            clientResult = trim(clientResult);  // Trim any unwanted whitespace/newline characters

            cout << "Client result: '" << clientResult << "', Expected result: '" << result << "'" << endl;

            // Handle result comparison based on the type of calculation
            if (calculationParts[0].find("f") != string::npos) {  // Floating-point operation
                double expectedResult = stod(result);
                double clientResultDouble = stod(clientResult);
                if (areFloatsEqual(expectedResult, clientResultDouble)) {
                    cout << "Client provided correct result. Sending 'OK'." << endl;
                    send(comm_socketfd, okMessage, sizeof(okMessage), 0);
                } else {
                    cout << "Client provided incorrect result. Sending 'ERROR'." << endl;
                    send(comm_socketfd, errorMessage, sizeof(errorMessage), 0);
                }
            } else {  // Integer operation
                int expectedIntResult = stoi(result);
                int clientIntResult = stoi(clientResult);
                if (expectedIntResult == clientIntResult) {
                    cout << "Client provided correct result. Sending 'OK'." << endl;
                    send(comm_socketfd, okMessage, sizeof(okMessage), 0);
                } else {
                    cout << "Client provided incorrect result. Sending 'ERROR'." << endl;
                    send(comm_socketfd, errorMessage, sizeof(errorMessage), 0);
                }
            }

            delete[] result;
            close(comm_socketfd);
        }
    }

    return 0;
}

int initializeSocket(const string& ip, int port, int& ipstatus) {
    int socketfd;
    struct addrinfo hints = {}, *res, *p;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip.c_str(), NULL, &hints, &res) != 0) {
        perror("Getaddrinfo error");
        exit(1);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in*)p->ai_addr;
            ipv4->sin_port = htons(port);
            socketfd = socket(AF_INET, SOCK_STREAM, 0);
            if (bind(socketfd, (struct sockaddr*)ipv4, sizeof(*ipv4)) == 0) {
                ipstatus = 1;
                break;
            }
        } else if (p->ai_family == AF_INET6) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*)p->ai_addr;
            ipv6->sin6_port = htons(port);
            socketfd = socket(AF_INET6, SOCK_STREAM, 0);
            if (bind(socketfd, (struct sockaddr*)ipv6, sizeof(*ipv6)) == 0) {
                ipstatus = 2;
                break;
            }
        }
    }

    freeaddrinfo(res);

    if (ipstatus == 0 || listen(socketfd, 5) < 0) {
        perror("Socket setup error");
        exit(1);
    }

    cout << "Listening on " << ip << " port " << port << endl;
    return socketfd;
}

char* calculateResult(const string& operation, double a, double b) {
    char* resultStr = new char[30];

    if (operation == "fsub") {
        sprintf(resultStr, "%8.8g\n", a - b);
    } else if (operation == "fmul") {
        sprintf(resultStr, "%8.8g\n", a * b);
    } else if (operation == "fadd") {
        sprintf(resultStr, "%8.8g\n", a + b);
    } else if (operation == "fdiv") {
        if (b != 0) {
            sprintf(resultStr, "%8.8g\n", a / b);
        } else {
            strcpy(resultStr, "Error: Division by zero\n");
        }
    } else if (operation == "sub") {
        sprintf(resultStr, "%d\n", static_cast<int>(a - b));
    } else if (operation == "mul") {
        sprintf(resultStr, "%d\n", static_cast<int>(a * b));
    } else if (operation == "add") {
        sprintf(resultStr, "%d\n", static_cast<int>(a + b));
    } else if (operation == "div") {
        if (b != 0) {
            sprintf(resultStr, "%d\n", static_cast<int>(a / b));
        } else {
            strcpy(resultStr, "Error: Division by zero\n");
        }
    }

    return resultStr;
}

vector<string> split(const string& s, const string& delimiter) {
    vector<string> tokens;
    string copy = s;  // Work with a non-const copy
    size_t start = 0, end;
    while ((end = copy.find(delimiter, start)) != string::npos) {
        tokens.push_back(copy.substr(start, end - start));
        start = end + delimiter.length();
    }
    tokens.push_back(copy.substr(start));  // Add last token
    return tokens;
}

string generateCalculationString() {
    string oper("fadd");
    double float1 = 64, float2 = 31;
    return oper + " " + to_string(float1) + " " + to_string(float2);
}
