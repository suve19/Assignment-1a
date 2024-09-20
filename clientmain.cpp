#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <math.h>
/* You will to add includes here */

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
// #define DEBUG

// Helper function to perform the requested operation and store the result.
void calculate_result(const char *operation, const char *value1, const char *value2, char *result) {
    double val1, val2, res;

    // Determine if the operation is on floating-point or integer numbers.
    if (operation[0] == 'f') {  // Floating-point operations have a 'f' prefix .(e.g., fadd, fsub)
        val1 = atof(value1);     // Convert strings to floats.
        val2 = atof(value2);
    } else {  // Integer operations
        val1 = atoi(value1);     // Convert strings to integers.
        val2 = atoi(value2);
    }

    // Perform the calculation based on the operation type.
    if (strcmp(operation, "add") == 0) {
        res = val1 + val2;
        sprintf(result, "%d\n", (int)res);  // Store result as an integer.
    } else if (strcmp(operation, "sub") == 0) {
        res = val1 - val2;
        sprintf(result, "%d\n", (int)res);
    } else if (strcmp(operation, "mul") == 0) {
        res = val1 * val2;
        sprintf(result, "%d\n", (int)res);
    } else if (strcmp(operation, "div") == 0) {
        if (val2 == 0) {
            sprintf(result, "ERROR\n");  // Handle division by zero error for integers.
        } else {
            res = val1 / val2;
            sprintf(result, "%d\n", (int)res);  // Store the result of integer division.
        }
    } else if (strcmp(operation, "fadd") == 0) {
        res = val1 + val2;
        sprintf(result, "%8.8g\n", res);  // Store floating-point result.
    } else if (strcmp(operation, "fsub") == 0) {
        res = val1 - val2;
        sprintf(result, "%8.8g\n", res);
    } else if (strcmp(operation, "fmul") == 0) {
        res = val1 * val2;
        sprintf(result, "%8.8g\n", res);
    } else if (strcmp(operation, "fdiv") == 0) {
        if (fabs(val2) < 0.0001) {  // Handle floating-point division by near-zero.
            sprintf(result, "ERROR\n");
        } else {
            res = val1 / val2;
            sprintf(result, "%8.8g\n", res);
        }
    } else {
        sprintf(result, "ERROR\n");  // Handle unknown operations.
    }
}

// Function to validate the protocol response from the server.
int validate_protocol_buffer(const char *buffer) {
    // Check if the buffer ends with two newlines, indicating the end of protocol versions.
    size_t len = strlen(buffer);
    if (len < 2 || strcmp(&buffer[len - 2], "\n\n") != 0) {
        printf("Buffer does not end with an empty newline.\n");
        return 0;
    }

    // Create a copy of the buffer to split it into lines.
    char *buf_copy = strdup(buffer);
    char *line = strtok(buf_copy, "\n");

    int valid_protocol_found = 0;

    // Loop through each line to check for valid "TEXT TCP" versions.
    while (line != NULL) {
        if (strncmp(line, "TEXT TCP", 8) == 0) {  // Check if the line starts with "TEXT TCP".
            valid_protocol_found = 1;  // Set flag if a valid protocol is found.
        }
        line = strtok(NULL, "\n");  // Move to the next line.
    }

    free(buf_copy);  // Free the dynamically allocated memory.

    // Ensure that at least one valid protocol was found.
    if (!valid_protocol_found) {
        printf("No valid protocol version found.\n");
        return 0;
    }

    return 1;  // All checks passed.
}

int main(int argc, char *argv[]){

  /*
    Read first input, assumes <ip>:<port> syntax, convert into one string (Desthost) and one integer (port). 
     Atm, works only on dotted notation, i.e. IPv4 and DNS. IPv6 does not work if its using ':'. 
  */
  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);
  // *Desthost now points to a sting holding whatever came before the delimiter, ':'.
  // *Dstport points to whatever string came after the delimiter. 

  int port=atoi(Destport);

  if (port <= 0) {
        fprintf(stderr, "Invalid port number.\n");
        return 1;
    }

  printf("Host: %s, Port: %d\n", Desthost, port);

  // Create a socket for the TCP connection using SOCK_STREAM.
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
      perror("socket");
      return 1;
  }

  // Set up the server address structure.
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);  // Convert port to network byte order.

  // Convert the IP address to binary form and store it in servaddr.
  if (inet_pton(AF_INET, Desthost, &servaddr.sin_addr) <= 0) {
      perror("inet_pton");
      close(sockfd);
      return 1;
  }

  // Attempt to connect to the server.
  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
      perror("connect");
      close(sockfd);
      return 1;
  }

// Retrieve and print the local IP and port after connection.
  struct sockaddr_in localaddr;
  socklen_t addrlen = sizeof(localaddr);
  if (getsockname(sockfd, (struct sockaddr *)&localaddr, &addrlen) == -1) {
      perror("getsockname");
      close(sockfd);
      return 1;
  }

  char local_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &localaddr.sin_addr, local_ip, INET_ADDRSTRLEN);
  int local_port = ntohs(localaddr.sin_port);

#ifdef DEBUG
    printf("Connected to %s:%d from local %s:%d\n", Desthost, port, local_ip, local_port);
#endif

  // Buffer to read data from the server.
  char buffer[1024];
  ssize_t n;

  // Read the protocol version(s) from the server.
  n = read(sockfd, buffer, sizeof(buffer) - 1);


  if (n > 100) {  // Check if the received bytes exceed 100, indicating a chargen server.
    printf("Detected chargen server, closing connection.\n");
    close(sockfd);
    return 1;
  }
  buffer[n] = '\0';  // Null-terminate the buffer.
  
  // Validate the protocol message from the server.
  if (validate_protocol_buffer(buffer)) {
    // Send "OK" to the server if the protocol is valid.
    const char *ok_message = "OK\n";
    ssize_t sent = write(sockfd, ok_message, strlen(ok_message));
    
    // Read the assigned operation and values from the server.
    n = read(sockfd, buffer, sizeof(buffer) - 1);
    
    buffer[n] = '\0';  // Null-terminate the buffer.
    printf("ASSIGNMENT: %s", buffer);

    // Parse the operation and values.
    char operation[16], value1[32], value2[32];
    sscanf(buffer, "%s %s %s", operation, value1, value2);

    // Calculate the result based on the operation.
    char result[64];
    calculate_result(operation, value1, value2, result);

    // Send the result back to the server.
    sent = write(sockfd, result, strlen(result));

#ifdef DEBUG
    printf("Calculated the result: %s", result);
#endif

    // Read the server's final response (OK or ERROR).
    n = read(sockfd, buffer, sizeof(buffer) - 1);

    buffer[n] = '\0';  // Null-terminate the buffer.

    // Print the final confirmation
    result[strcspn(result, "\n")] = '\0';  // Remove the trailing newline from the result.
    printf("OK (myresult=%s)\n", result);

    // Close the connection after the exchange is complete.
    close(sockfd);
  } else {
    printf("Unexpected message from server: %s", buffer);
    close(sockfd);
    return 1;
  }
  return 0;
}
