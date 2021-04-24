/*****************************************************************************
 * client-c-sol.c                                                                 
 * Name:
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <err.h>

#define SEND_BUFFER_SIZE 2048


/* TODO: client()
 * Open socket and send message from stdin.
 * Return 0 on success, non-zero on failure
*/
int client(char *server_ip, char *server_port) {
  // declare variables
  int sockfd, new_sockfd; // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_in client_addr; // client's address information
  socklen_t sin_size;
  int error;
  int yes = 1;                  // used in setsockopt()
  char *status;
  char ipAddr[INET_ADDRSTRLEN]; // number of byte for IPv4 address
  unsigned char buffer[SEND_BUFFER_SIZE], serialize_buf[SEND_BUFFER_SIZE];

  // build address data structure with getaddrinfo()
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET; //AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  error = getaddrinfo(server_ip, server_port, &hints, &servinfo);
  if (error) {
    errx(1, "%s", gai_strerror(error));
  }

  // create a socket. here, s is the new socket descriptor
  if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
                       servinfo->ai_protocol)) < 0) {
    // handle the error if failed to create a socket
    perror("server: socket");
    exit(-1);
  }  
  
  // conncet to the server using the socket descriptor s
  // Note: different from the server, the client does NOT create
  // another socket descriptor for communication
  if(connect(sockfd, servinfo->ai_addr,  servinfo->ai_addrlen) < 0) {
    // handle the error if failed to connect
    perror("server: connect");
    exit(-1);
  }

  int size;
  //while(size=fread(buffer, 1, SEND_BUFFER_SIZE, stdin)) {
  while((size=read(0, buffer, SEND_BUFFER_SIZE))) {
    if(send(sockfd, buffer, size, 0) < 0) {
      perror("client: send");
    }
  }
  
  // Done, close the s socket descriptor
  close(sockfd);

  return 0;
}

/*
 * main()
 * Parse command-line arguments and call client function
*/
int main(int argc, char **argv) {
  char *server_ip;
  char *server_port;

  if (argc != 3) {
    fprintf(stderr, "Usage: ./client-c [server IP] [server port] < [message]\n");
    exit(EXIT_FAILURE);
  }

  server_ip = argv[1];
  server_port = argv[2];
  return client(server_ip, server_port);
}
