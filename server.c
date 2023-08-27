#include "server.h"

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

typedef enum Error { INVALID_PORT = 1, DUPLICATE_PATH } Error;
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
  int sockfd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sigaction sa;
  int yes = 1;

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

  int BUF_SIZE = 4096;
  char buffer[BUF_SIZE];

  snprintf(buffer, BUF_SIZE, "HTTP/1.1 %d %s\r\n", res.status_code,
           get_phrase(res.status_code));
  append_to_str(&response_raw, buffer);

  // Server defined headers
  // TODO: Give user option to override these, for now they're static
  snprintf(buffer, BUF_SIZE, "Server: %s\r\n", SERVER_NAME);
  append_to_str(&response_raw, buffer);

  snprintf(buffer, BUF_SIZE, "Date: %s\r\n", get_current_time_str());
  append_to_str(&response_raw, buffer);

  snprintf(buffer, BUF_SIZE, "Content-Length: %ld\r\n", strlen(res.body));
  append_to_str(&response_raw, buffer);

  for (int i = 0; i < res.num_headers; i++) {
    snprintf(buffer, BUF_SIZE, "%s: %s\r\n", res.headers[i].name,
             res.headers[i].value);
    append_to_str(&response_raw, buffer);
  }

  append_to_str(&response_raw, "\r\n");
  append_to_str(&response_raw, res.body);

  return send(sd, response_raw, strlen(response_raw), 0);
}

http_response response(int status_code, char *body) {
  http_response res;
  res.body = body;
  res.num_headers = 0;
  res.status_code = status_code;

  return res;
}

http_response text_response(char *s) {
  http_response res;
  res.body = s;
  res.num_headers = 0;
  res.status_code = 200;

  return res;
}

http_response err_500(char *error) {
  http_response res;
  res.body = error;
  res.num_headers = 0;
  res.status_code = 500;

  return res;
}

http_response file_response(char *file_name) {
  http_response res;
  FILE *file = fopen(file_name, "rb");  // Open the file in binary mode

  if (file == NULL) {
    if (errno == ENOENT) return response(404, "File not found.");

    perror("Error opening the file");
    return err_500("Could not read file");
  }

  // Determine the file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Allocate memory for the buffer
  char *buffer = (char *)malloc(file_size + 1);  // +1 for the null terminator

  if (buffer == NULL) {
    perror("Memory allocation error");
    return err_500("Could not malloc buffer");
  }

  // Read the entire file into the buffer
  size_t read_size = fread(buffer, 1, file_size, file);

  if (read_size != file_size) {
    perror("Error reading the file");
    return err_500("Could not read file");
    free(buffer);
  }

  buffer[file_size] = '\0';  // Null-terminate the buffer

  // Close the file
  fclose(file);

  res.body = buffer;
  res.num_headers = 0;
  res.status_code = 200;

  char *last_modified = get_last_modified_date(file_name);
  if (last_modified != NULL)
    set_res_header(&res, "Last-Modified", last_modified);

  return res;
}

bool matchPattern(const char *pattern, const char *text) {
  int patternLen = strlen(pattern);
  int textLen = strlen(text);
  int patternIndex = 0;
  int textIndex = 0;

  while (patternIndex < patternLen && textIndex < textLen) {
    if (pattern[patternIndex] == ':') {
      // Found a placeholder in the pattern
      patternIndex++;  // Skip the ':'
      while (pattern[patternIndex] != '/' && pattern[patternIndex] != '\0') {
        if (pattern[patternIndex] != text[textIndex]) {
          return false;  // Mismatch in placeholder
        }
        patternIndex++;
        textIndex++;
      }
    } else {
      // Regular character comparison
      if (pattern[patternIndex] != text[textIndex]) {
        return false;  // Mismatch in regular characters
      }
      patternIndex++;
      textIndex++;
    }
  }

  // Check if both strings reached the end
  if (patternIndex == patternLen && textIndex == textLen) {
    return true;  // Matched completely
  }

  return false;  // One of the strings is not fully matched
}

bool resolve_route(http_request req, router rtr) {
  bool resolved = false;
  for (int i = 0; i < rtr.num_routes; i++) {
    if (rtr.routes[i].type == HANDLER_ROUTE &&
        strcmp(req.path, rtr.routes[i].path) == 0) {
      send_http_response(req.sd, rtr.routes[i].handler(req));
      resolved = true;
      break;
    }
    if (rtr.routes[i].type == STATIC_DIR &&
        strncmp(req.path, rtr.routes[i].path, strlen(rtr.routes[i].path)) ==
            0) {
      // f
      char *strt = req.path + strlen(rtr.routes[i].path);
      char *res = strdup(rtr.routes[i].base_dir);
      strcat(res, strt);
      send_http_response(req.sd, file_response(res));
    }
  }
  return resolved;
}

// client connection
void handle_request(int sd, router rtr) {
  int rcvd;
  buf = malloc(65535);
  rcvd = recv(sd, buf, 65535, 0);

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

    bool resolved = resolve_route(*req, rtr);

    if (!resolved) {
      http_response not_found_res;
      not_found_res.status_code = 404;
      not_found_res.num_headers = 0;
      send_http_response(sd, not_found_res);
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

// TODO: support multiple routers
int init_server(char *port, router r) {
  int sockfd = establish_listening_socket(port);
  int new_fd;
  char s[INET6_ADDRSTRLEN];

  socklen_t sin_size;
  struct sockaddr_storage their_addr;  // connector's address information

  printf("Established server on port %s...\n", port);

  int pid;
  while (1) {  // main accept() loop
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("[SERVER] got connection from %s\n", s);

    // TODO: Probably horrible idea, but will do for now
    // Either do proper multithreading or use select() syscall
    pid = fork();
    if (pid < 0) {
      perror("ERROR in creating child process");
      exit(1);
    }
    if (pid == 0) {
      handle_request(new_fd, r);
    }
  }
}

int init_router(router *r) {
  r->num_routes = 0;
  return 0;
}

bool used_path(router *r, char *path) {
  for (int i = 0; i < r->num_routes; i++) {
    if (strcmp(r->routes[i].path, path) == 0) {
      return true;
    }
  }
  return false;
}

void register_route(router *r, char *path,
                    http_response (*handler)(http_request req)) {
  if (used_path(r, path)) {
    fprintf(stderr, "[ERROR] Duplicate path (%s) detected\n", path);
    exit(DUPLICATE_PATH);
  }
  r->routes[r->num_routes].type = HANDLER_ROUTE;
  r->routes[r->num_routes].handler = handler;
  r->routes[r->num_routes].path = path;
  r->num_routes++;
}

void register_static_dir(router *r, char *path, char *base_dir) {
  if (used_path(r, path)) {
    fprintf(stderr, "[ERROR] Duplicate path (%s) detected\n", path);
    exit(DUPLICATE_PATH);
  }
  r->routes[r->num_routes].type = STATIC_DIR;
  r->routes[r->num_routes].path = path;
  r->routes[r->num_routes].base_dir = base_dir;
  r->num_routes++;
}