
#include "flasc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct http_code_pair http_pairs[] = {{100, "Continue"},
                                      {101, "Switching protocols"},
                                      {102, "Processing"},
                                      {103, "Early Hints"},
                                      {200, "OK"},
                                      {201, "Created"},
                                      {202, "Accepted"},
                                      {203, "Non-Authoritative Information"},
                                      {204, "No Content"},
                                      {205, "Reset Content"},
                                      {206, "Partial Content"},
                                      {207, "Multi-Status"},
                                      {208, "Already Reported"},
                                      {226, "IM Used"},
                                      {300, "Multiple Choices"},
                                      {301, "Moved Permanently"},
                                      {302, "Found"},
                                      {303, "See Other"},
                                      {304, "Not Modified"},
                                      {305, "Use Proxy"},
                                      {306, "Switch Proxy"},
                                      {307, "Temporary Redirect"},
                                      {308, "Permanent Redirect"},
                                      {400, "Bad Request"},
                                      {401, "Unauthorized"},
                                      {402, "Payment Required"},
                                      {403, "Forbidden"},
                                      {404, "Not Found"},
                                      {405, "Method Not Allowed"},
                                      {406, "Not Acceptable"},
                                      {407, "Proxy Authentication Required"},
                                      {408, "Request Timeout"},
                                      {409, "Conflict"},
                                      {410, "Gone"},
                                      {411, "Length Required"},
                                      {412, "Precondition Failed"},
                                      {413, "Payload Too Large"},
                                      {414, "URI Too Long"},
                                      {415, "Unsupported Media Type"},
                                      {416, "Range Not Satisfiable"},
                                      {417, "Expectation Failed"},
                                      {418, "I'm a Teapot"},
                                      {421, "Misdirected Request"},
                                      {422, "Unprocessable Entity"},
                                      {423, "Locked"},
                                      {424, "Failed Dependency"},
                                      {425, "Too Early"},
                                      {426, "Upgrade Required"},
                                      {428, "Precondition Required"},
                                      {429, "Too Many Requests"},
                                      {431, "Request Header Fields Too Large"},
                                      {451, "Unavailable For Legal Reasons"},
                                      {500, "Internal Server Error"},
                                      {501, "Not Implemented"},
                                      {502, "Bad Gateway"},
                                      {503, "Service Unavailable"},
                                      {504, "Gateway Timeout"},
                                      {505, "HTTP Version Not Supported"},
                                      {506, "Variant Also Negotiates"},
                                      {507, "Insufficient Storage"},
                                      {508, "Loop Detected"},
                                      {510, "Not Extended"},
                                      {511, "Network Authentication Required"}};

char *strtoke(char *str, const char *delim) {
  static char *start = NULL; /* stores string str for consecutive calls */
  char *token = NULL;        /* found token */
  /* assign new start in case */
  if (str) start = str;
  /* check whether text to parse left */
  if (!start) return NULL;
  /* remember current start as found token */
  token = start;
  /* find next occurrence of delim */
  start = strpbrk(start, delim);
  /* replace delim with terminator and move start to follower */
  if (start) *start++ = '\0';
  /* done */
  return token;
}

// TODO: If host only sends \n, disgard the \r but still accept the request
// TODO: Error handeling
int parse_http_request(char *raw_request, http_request *request) {
  request->body = "This is a test";
  char *method, *uri, *prot, *qs, *payload;
  int payload_size;
  method = strtoke(raw_request, " ");
  uri = strtoke(NULL, " ");
  prot = strtoke(NULL, "\r\n");
  strtoke(NULL, "\r\n");

  request->method = method;
  // TODO: QUERY PARAMS
  request->path = uri;

  char *header_str;
  request->num_headers = 0;
  while ((header_str = strtoke(NULL, "\r\n")) != NULL) {
    header reqhdr;
    char *colon = strchr(header_str, ':');
    if (colon != NULL) {
      *colon = '\0';
      reqhdr.name = header_str;
      reqhdr.value = colon + 2;  // Skip ": "

      request->headers[request->num_headers] = reqhdr;
      request->num_headers++;
      strtoke(NULL, "\r\n");
    } else {
      strtoke(NULL, "\r\n");
      request->body = strtoke(NULL, "\r\n");
      break;
    }
  }
  return 0;
}

char *get_header(http_request req, char *header_name) {
  for (int i = 0; i < req.num_headers; i++) {
    if (strcmp(req.headers[i].name, header_name) == 0) {
      return req.headers[i].value;
    }
  }
  return NULL;
}

void set_header(http_request *req, char *header_name, char *value) {
  for (int i = 0; i < req->num_headers; i++) {
    if (strcmp(req->headers[i].name, header_name) == 0) {
      req->headers[i].value = value;
      return;
    }
  }
  // Header not added yet, create a new one.
  req->headers[req->num_headers].name = header_name;
  req->headers[req->num_headers].value = value;
  req->num_headers++;
}

char *get_phrase(int code) {
  int num_pairs = sizeof(http_pairs) / sizeof(http_pairs[0]);
  for (int i = 0; i < num_pairs; i++) {
    if (http_pairs[i].code == code) {
      return http_pairs[i].phrase;
    }
  }
  return NULL;
}
