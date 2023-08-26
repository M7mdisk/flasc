#ifndef FLASC_H
#define FLASC_H
#define MAX_HEADERS 100
#define MAX_ROUTES 100

typedef struct header {
  char* name;
  char* value;
} header;

typedef struct http_request {
  int sd;
  char *method, *path;
  char* body;
  header headers[MAX_HEADERS];
  int num_headers;
} http_request;

int parse_http_request(char* raw_request, http_request* request);
// TODO: I don't think this is needed but keep for now
void free_http_request(http_request request);

char* get_header(http_request req, char* header_name);
void set_header(http_request* req, char* header_name, char* value);

typedef struct http_response {
  int status_code;
  char* body;
  header headers[MAX_HEADERS];
  int num_headers;
} http_response;

/* IDEA: Add middleware support (something ran before actual handler)
http_res (*middleware)(http_req req, http_res(*next)(http_req req));
*/
// IDEA: Error handler function
// http_response (*error_handler)(http_request req, int error_code);
typedef struct router {
  struct {
    char* path;
    http_response (*handler)(http_request req);
    // TODO: Routes should not have limit, dynamic allowcation and keep counter
    // instead
  } routes[MAX_ROUTES];
} router;

struct http_code_pair {
  int code;
  char* phrase;
};

char* get_phrase(int code);
extern struct http_code_pair http_pairs[];

#endif