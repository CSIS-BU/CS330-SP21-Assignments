/*****************************************************************************
 * client.c                                                                 
 * Name: Tachun Lin
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

#define SEND_BUFFER_SIZE 1024

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
  char message[SEND_BUFFER_SIZE-44];
} data;


/* TODO: client()
 * Open socket and send message from stdin.
 * Return 0 on success, non-zero on failure
*/
int client(char *server_ip, char *server_port, char *router_ip, char *router_port) {
  // declare variables
  int sockfd; // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo;
  int error;
  char buffer[SEND_BUFFER_SIZE-44];

  // build address data structure with getaddrinfo()
  memset(&hints, 0, sizeof hints);
  // use AF_INET for IPv4
  hints.ai_family = AF_INET; //AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  error = getaddrinfo(router_ip, router_port, &hints, &servinfo);
  if (error) {
    errx(1, "%s", gai_strerror(error));
  }

  // create a socket. here, s is the new socket descriptor
  if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
                       servinfo->ai_protocol)) < 0) {
    // handle the error if failed to create a socket
    perror("client: socket");
    exit(-1);
  }  
  
  // conncet to the server using the socket descriptor s
  // Note: different from the server, the client does NOT create
  // another socket descriptor for communication
  if(connect(sockfd, servinfo->ai_addr,  servinfo->ai_addrlen) < 0) {
    // handle the error if failed to connect
    perror("client: connect");
    exit(-1);
  }

  /*
   * Note: I run the programs in the virtual machines, thus
   *       the client has the following 10.0.0.1 address.
   *       In this sample solution, the server would send
   *       a message back to the client, so what we put as
   *       the client address/port does not really matter.
   */
  data mesg;
  strcpy(mesg.client_ip, "10.0.0.1");
  strcpy(mesg.client_port, "11111");
  strcpy(mesg.server_ip, server_ip);
  strcpy(mesg.server_port, server_port);

  size_t size;
  //while(size=fread(buffer, 1, SEND_BUFFER_SIZE, stdin)) {
  while((size=read(0, buffer, SEND_BUFFER_SIZE-44))) {
    strncpy(mesg.message, buffer, size);
    if(send(sockfd, (void*) &mesg, sizeof mesg, 0) < 0) {
      perror("client: send");
    }
    /*
     * The following is just to show you the information
     * sent.
     */
    printf("[client] server ip=%s\n", mesg.server_ip);
    printf("[client] server port=%s\n", mesg.server_port);
    printf("[client] client ip=%s\n", mesg.client_ip);
    printf("[client] client port=%s\n", mesg.client_port);
    printf("[client] message sent=%s", mesg.message);
  }
  printf("===== end of the message =====\n");
  
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
  char *router_ip;
  char *router_port;

  if (argc != 5) {
    fprintf(stderr, "Usage: ./client-c [server IP] [server port] [router IP] [router port] < [message]\n");
    exit(EXIT_FAILURE);
  }

  server_ip = argv[1];
  server_port = argv[2];
  router_ip = argv[3];
  router_port = argv[4];

  return client(server_ip, server_port, router_ip, router_port);
}
