#ifndef FLASC_H
#define FLASC_H
#define MAX_HEADERS 100
#define MAX_PARAMS 100
#define MAX_ROUTES 100

typedef struct Pair {
  char* name;
  char* value;
} Pair;

typedef Pair header;
typedef Pair query_param;

typedef struct http_request {
  int sd;
  char *method, *path;
  char* body;
  header headers[MAX_HEADERS];
  query_param params[MAX_PARAMS];
  int num_headers;
  int num_params;
} http_request;

int parse_http_request(char* raw_request, http_request* request);
// TODO: I don't think this is needed but keep for now
void free_http_request(http_request request);

char* get_req_header(http_request req, char* header_name);
void set_req_header(http_request* req, char* header_name, char* value);

typedef struct http_response {
  int status_code;
  char* body;
  header headers[MAX_HEADERS];
  int num_headers;
} http_response;

struct http_code_pair {
  int code;
  char* phrase;
};

void set_res_header(http_response* res, char* header_name, char* value);

char* get_phrase(int code);
extern struct http_code_pair http_pairs[];

#endif