#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"

bool is_positive_int(char *s) {
  for (int i = 0; s[i] != '\0'; i++) {
    if (!isdigit(s[i])) return false;
  }
  return true;
}

http_response hello(http_request req) {
  if (strcmp(req.method, "GET") == 0) {
    return text_response("Hello, world!\n");
  } else if (strcmp(req.method, "POST") == 0) {
    char *name = req.body;
    char res[50];
    snprintf(res, 50, "Hello, %s!\n", name);
    return text_response(res);
  }
  return text_response("Try using GET or POST\n");
}

http_response file_example(http_request req) {
  return file_response("examples/index.html");
}

int main(int argc, char **argv) {
  char *PORT = "8080";

  if (argc > 1) {
    if (is_positive_int(argv[1])) {
      PORT = argv[1];
    } else {
      fprintf(stderr, "Invalid port number. Usage: %s [PORT]\n", argv[0]);
      exit(1);
    }
  }

  router r;
  init_router(&r);
  register_route(&r, "/hello", hello);
  register_route(&r, "/index", file_example);

  init_server(PORT, r);
}