#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "server.h"

bool is_positive_int(char *s) {
  for (int i = 0; s[i] != '\0'; i++) {
    if (!isdigit(s[i])) return false;
  }
  return true;
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
  init_server(PORT);
}