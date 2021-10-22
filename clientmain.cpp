#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sstream>
#define DEBUG
#define PORT 8080
#define SA struct sockaddr
#include <cstring>

// communication function
void communication_with_server(int sockfd)
{
    char buff[1450];
    char operation[4];
    int i1,i2,iresult,server_iresult;
    float f1,f2,fresult,server_fresult;
    std::string tmp;
    // function for communicating with the server
    read(sockfd, buff, sizeof(buff));
    printf("%s\n", buff);
    printf("\n");
    if ((strncmp(buff, "TEXT TCP 1.0", 13)) == 0) {
     		printf("OK\n");
	}
	else {
    		printf("Protocol Not Supported");
	     }
	printf("\n Waiting for operation...");
        read(sockfd, buff, sizeof(buff));
        printf("\nRecived the following operation from server : \n %s \n", buff);

        char * buff_split = strtok(buff, " ");
        int cntr = 0;
       while( buff_split != NULL ) 
       {	
	         if ( cntr == 0 )
                 {
                            std::stringstream val(buff_split);
                            val >> operation;
                 }
      				
     		 if ( cntr == 1 )
                  {
                            std::stringstream val(buff_split);
                            if ( operation[0] == 'f' )
		            val >> f1;
			    else
			    val >> i1;
                  }
                  if ( cntr == 2 ) 
                  {
                 	     std::stringstream val(buff_split);	
                           		 if ( operation[0] == 'f' )
                            		 	val >> f2;
                            		else
                            			val >> i2;

		   }
		 if ( cntr == 4)
		 {
                            std::stringstream val(buff_split);
                            if ( operation[0] == 'f' )
                                val >> server_fresult;
                            else
                                val >> server_iresult;

                }	         																						
    	 	 buff_split = strtok(NULL, " ");
                cntr = cntr + 1;
       }


      if(strcmp(operation,"fadd")==0){
                        fresult=f1+f2;
                        } else if (strcmp(operation, "fsub")==0){
                                        fresult=f1-f2;
                                } else if (strcmp(operation, "fmul")==0){
                                                fresult=f1*f2;
                                        } else if (strcmp(operation, "fdiv")==0){
                                                        fresult=f1/f2;
                                                }
                tmp=std::to_string(fresult);
                strcpy(buff,tmp.c_str());
                write(sockfd,buff,sizeof(buff));

       if(strcmp(operation,"add")==0){
                      iresult=i1+i2;
                    } else if (strcmp(operation, "sub")==0){
                                      iresult=i1-i2;
                            } else if (strcmp(operation, "mul")==0){
                                              iresult=i1*i2;
                                    } else if (strcmp(operation, "div")==0){
                                                      iresult=i1/i2;
                                                }

	        tmp=std::to_string(iresult);
                strcpy(buff,tmp.c_str());
                write(sockfd,buff,sizeof(buff));

                printf("\n Server Response for Operation Result sent by Client : ");
	        read(sockfd,buff,sizeof(buff));
                printf("\n%s",buff);

      exit(0);
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

  /* Do magic */
  int port=atoi(Destport);
#ifdef DEBUG 
  printf("Host %s, and port %d.\n",Desthost,port);
#endif

    int sockfd;
    struct sockaddr_in servaddr;
    // socket create and varification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Could not create Socket ..Fatal Error\n");
        exit(0);
    }
    else
        printf("Succesful in creating Socket \n");
    bzero(&servaddr, sizeof(servaddr));
   
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
   
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else {
        printf("connected to the server..\n");
        communication_with_server(sockfd);
    }
    // close the socket
    close(sockfd);

  
}
