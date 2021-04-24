/*****************************************************************************
 * server.c
 * Name: Tachun Lin
 *****************************************************************************/

#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define QUEUE_LENGTH 10
#define RECV_BUFFER_SIZE 1024 

/*
 * Note: Why do I use the following structure?
 *       The IP address would use up to "xxx.xxx.xxx.xxx"
 *       characters, in total 15 characters. And we keep
 *       1 ending null character for the string.
 *       Similarly, the port number can go up to 65535,
 *       thus 5+1 characters.
 * See: http://www.cs.ecu.edu/karl/2530/spr17/Notes/C/String/nullterm.html
 */

typedef struct
{
  char client_ip[16];
  char client_port[6];
  char server_ip[16];
  char server_port[6];
  char message[RECV_BUFFER_SIZE-44];
} data;


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/* TODO: server()
 * Open socket and wait for the router to connect
 * Print received message to stdout
 * Return 0 on success, non-zero on failure
 */
int server(char *server_port) {
  // declare variables
  int sockfd, new_sockfd; // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo;
  struct sockaddr_in client_addr; // client's address information
  socklen_t length;
  int error;
  size_t size;
  int yes = 1;                  // used in setsockopt()
  char buffer[RECV_BUFFER_SIZE];

  // build address data structure with getaddrinfo()
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET; //AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  error = getaddrinfo(NULL, server_port, &hints, &servinfo);
  if (error) {
    errx(1, "%s", gai_strerror(error));
  }

  // create a socket. here, s is the new socket descriptor
  if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
                       servinfo->ai_protocol)) < 0) {
    // handle the error if failed to create a socket
    perror("server: socket error");
    exit(-1);
  }

  // the following overcomes the bind() error "address already in use"
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(-1);
  }

  // bind the socket to the port we passed in to getaddrinfo()
  if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
    // handle the error if failed to bind
    close(sockfd);
    perror("server: bind error\n");
    exit(-1);
  }
  freeaddrinfo(servinfo);

  if (servinfo == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    exit(-1);
  }

  // prepare the server for incoming requests.
  // you may consider this step as getting the reception ready
  if (listen(sockfd, QUEUE_LENGTH) < 0) {
    perror("server: listen error");
    exit(-1);
  }

  // wait for connection, then start exchanging messages
  while (1) {
    /*
     * Pay special attention here!!! the accept() call will
     * returns the newly connected socket descriptor new _ s,
     * which will be used between this pair of
     * (server addr, server port, client addr, client port)
     */
    length = sizeof client_addr;
    if ((new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr,
                             &length)) < 0) {
      // handle the error if failed to accept }
      perror("server: accept error");
      exit(-1); //continue;
    }

    /*
     * prepare the server for incoming message from the router
     * Note: this is a blocking function meaning that it will wait
     * for I/O to occur
     * Note: Here, we use the new_sockfd descriptor, NOT s!!!
     */

    while((size= recv(new_sockfd, buffer, RECV_BUFFER_SIZE, 0)) > 0) {
      //fwrite(buffer, 1, size, stdout);
      //write(1, buffer, size);
      data *mesg = (data*) buffer;
      printf("[server] server ip=%s\n", mesg->server_ip);
      printf("[server] server port=%s\n", mesg->server_port);
      printf("[server] client ip=%s\n", mesg->client_ip);
      printf("[server] client port=%s\n", mesg->client_port);
      printf("[server] message received=%s", mesg->message);
    }
    fflush(stdout);
    printf("===== end of message =====\n");
    
    /*
     * Done, close the new _ s descriptor, i.e., release the connection.
     * Note: you should NOT close the socket descriptor s
     */
    close(new_sockfd);
  }
  return 0;
}

/*
 * main():
 * Parse command-line arguments and call server function
 */
int main(int argc, char **argv) {
  char *server_port;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./server-c [server port]\n");
    exit(EXIT_FAILURE);
  }

  server_port = argv[1];
  return server(server_port);
}
