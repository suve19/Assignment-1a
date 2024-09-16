#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
/* You will to add includes here */

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
#define DEBUG


// Included to get the support library
#include <calcLib.h>

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

  /* Do magic */
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

}
