#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "flasc.h"
#include "utils.c"

#define BACKLOG 10
#define SERVER_NAME "Flasc 0.0.1"

typedef enum Error { INVALID_PORT = 1 } Error;
static char *buf;

// From Beej's guide to networking
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void sigchld_handler(int s) {
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}

int establish_listening_socket(char *PORT) {
  int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;  // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);  // Empty struct
  hints.ai_family = AF_UNSPEC;      // Use IPv4 or IPv6, doesn't matter
  hints.ai_socktype = SOCK_STREAM;  // TCP stream socket
  hints.ai_flags = AI_PASSIVE;      // use my IP

  int rv = getaddrinfo(NULL, PORT, &hints, &servinfo);
  if (rv != 0) {
    fprintf(stderr, "[ERROR] could not establish socket. getaddrinfo: %s\n",
            gai_strerror(rv));
    return rv;
  }

  // loop through all the results and bind to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo);  // all done with this structure

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler;  // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  return sockfd;
}

int send_http_response(int sd, http_response res) {
  char *response_raw = NULL;

  char buffer[4096];

  sprintf(buffer, "HTTP/1.1 %d %s\r\n", res.status_code,
          get_phrase(res.status_code));
  append_to_str(&response_raw, buffer);

  // Server defined headers
  // TODO: Give user option to override these, for now they're static
  sprintf(buffer, "Server: %s\r\n", SERVER_NAME);
  append_to_str(&response_raw, buffer);

  sprintf(buffer, "Date: %s\r\n", get_current_time_str());
  append_to_str(&response_raw, buffer);

  sprintf(buffer, "Content-Length: %d\r\n", strlen(res.body));
  append_to_str(&response_raw, buffer);

  for (int i = 0; i < res.num_headers; i++) {
    sprintf(buffer, "%s: %d\r\n", res.headers[i].name, res.headers[i].value);
    append_to_str(&response_raw, buffer);
  }

  append_to_str(&response_raw, "\r\n");
  append_to_str(&response_raw, res.body);
  printf("\n---\n%s\n---\n", response_raw);

  return send(sd, response_raw, strlen(response_raw), 0);
}

void text_response(http_request req, char *s) {
  http_response res;
  res.body = s;
  res.status_code = 200;

  send_http_response(req.sd, res);
}

// client connection
void handle_request(int sd) {
  int rcvd, fd, bytes_read;
  buf = malloc(65535);
  rcvd = recv(sd, buf, 65535, 0);

  char *method, uri, prot;
  if (rcvd < 0)  // receive error
    fprintf(stderr, ("recv() error\n"));
  else if (rcvd == 0)  // receive socket closed
    fprintf(stderr, "Client disconnected upexpectedly.\n");
  else  // message received
  {
    buf[rcvd] = '\0';  // Terminate message buffer.

    http_request *req = malloc(sizeof(http_request));
    req->sd = sd;
    int r = parse_http_request(buf, req);
    if (r != 0) {
      // TODO: Should this respond with a 500 instead?
      fprintf(stderr, "INVALID HTTP REQUEST, could not parse.");
      return;
    }

    printf("The req is a %s, req body is %s\n", req->method, req->body);

    // Route request
    // Poorman router for now, will implement router struct later with route
    // registration

    if (strcmp(req->path, "/html") == 0) {
      // file_response("test.html");
    } else if (strcmp(req->path, "/hello") == 0) {
      printf("Doing hello world!\n");
      text_response(*req, "Hello, World!");
    }

    free(req);
  }
  free(buf);
  // Closing SOCKET
  shutdown(
      sd,
      SHUT_RDWR);  // All further send and recieve operations are DISABLED...
  close(sd);
}

int main(int argc, char **argv) {
  char *PORT = "8080";

  if (argc > 1) {
    if (is_positive_int(argv[1])) {
      PORT = argv[1];
    } else {
      fprintf(stderr, "Invalid port number. Usage: %s [PORT]\n", argv[0]);
      exit(INVALID_PORT);
    }
  }
  int sockfd = establish_listening_socket(PORT);
  int new_fd;
  char s[INET6_ADDRSTRLEN];

  socklen_t sin_size;
  struct sockaddr_storage their_addr;  // connector's address information

  while (1) {  // main accept() loop
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: got connection from %s\n", s);

    handle_request(new_fd);
  }

  printf("Attempting to Establish server on port %s...\n", PORT);
}
