#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<unistd.h>


static int open_client_socket(char* const server_hostname, const char* server_service){
  int sockfd = -1;
  struct addrinfo* ai,* ai_sel = NULL,* ai_head;
  struct addrinfo hints;
  int err;

  /* 1) resolve hostname to IPV4 address*/
  hints.ai_flags = 0;
  hints.ai_family = AF_INET; /*ipv4*/
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_addrlen = 0;
  hints.ai_addr = NULL;
  hints.ai_canonname = NULL;
  hints.ai_next = NULL:


}
