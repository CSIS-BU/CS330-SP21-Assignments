/*****************************************************************************
 * router.c
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

/*
 * This is actually the original client code.
 */
void router_to_server(char *buffer) {
  // declare variables
  int sockfd; // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo;
  int error;
  data mesg;
  memcpy(&mesg, buffer, RECV_BUFFER_SIZE);

  // build address data structure with getaddrinfo()
  memset(&hints, 0, sizeof hints);
  // use AF_INET for IPv4
  hints.ai_family = AF_INET; //AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  error = getaddrinfo(mesg.server_ip, mesg.server_port, &hints, &servinfo);
  if (error) {
    errx(1, "%s", gai_strerror(error));
  }

  // create a socket. here, s is the new socket descriptor
  if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
                       servinfo->ai_protocol)) < 0) {
    // handle the error if failed to create a socket
    perror("router: socket");
    exit(-1);
  }  
  
  // conncet to the server using the socket descriptor s
  // Note: different from the server, the client does NOT create
  // another socket descriptor for communication
  if(connect(sockfd, servinfo->ai_addr,  servinfo->ai_addrlen) < 0) {
    // handle the error if failed to connect
    perror("router: connect");
    exit(-1);
  }

  /*
   * This is just to show that the router does receive the message
   * which contains the server/client IP/port and the message.
   */
  printf("[router] server ip=%s\n", mesg.server_ip);
  printf("[router] server port=%s\n", mesg.server_port);
  printf("[router] client ip=%s\n", mesg.client_ip);
  printf("[router] client port=%s\n", mesg.client_port);
  printf("[router] message received=%s", mesg.message);

  if(send(sockfd, &mesg, RECV_BUFFER_SIZE, 0) < 0) {
    perror("client: send");
  }
  
  printf("===== end of the message =====\n");
  // Done, close the s socket descriptor
  close(sockfd);
}



/* TODO: router()
 * Open socket and wait for client to connect

 */
int router(char *router_port) {
  // declare variables
  int sockfd, new_sockfd; // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo;
  struct sockaddr *client_addr;
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

  error = getaddrinfo(NULL, router_port, &hints, &servinfo);
  if (error) {
    errx(1, "%s", gai_strerror(error));
  }

  // create a socket. here, s is the new socket descriptor
  if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
                       servinfo->ai_protocol)) < 0) {
    // handle the error if failed to create a socket
    perror("router: socket error");
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
    perror("router: bind error\n");
    exit(-1);
  }
  freeaddrinfo(servinfo);

  if (servinfo == NULL) {
    fprintf(stderr, "router: failed to bind\n");
    exit(-1);
  }

  // prepare the router for incoming requests.
  // you may consider this step as getting the reception ready
  if (listen(sockfd, QUEUE_LENGTH) < 0) {
    perror("router: listen error");
    exit(-1);
  }

  // wait for connection, then start exchanging messages
  while (1) {
    /*
     * Pay special attention here!!! the accept() call will
     * returns the newly connected socket descriptor,
     * which will be used between this pair of
     * (router addr, router port, client addr, client port)
     */
    length = sizeof client_addr;
    if ((new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr,
                             &length)) < 0) {
      // handle the error if failed to accept }
      perror("router: accept error");
      exit(-1); //continue;
    }

    /*
     * prepare the router for incoming message from the client
     * Note: this is a blocking function meaning that it will wait
     * for I/O to occur
     * Note: Here, we use the new_sockfd descriptor, NOT s!!!
     */

    while((size= recv(new_sockfd, buffer, RECV_BUFFER_SIZE, 0)) > 0) {
      // create client socket for the connection between the router and
      // the server
      router_to_server(buffer);
    }
    //fflush(stdout);
    
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
 * Parse command-line arguments and call router function
 */
int main(int argc, char **argv) {
  char *router_port;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./router-c [router port]\n");
    exit(EXIT_FAILURE);
  }

  router_port = argv[1];
  return router(router_port);
}
