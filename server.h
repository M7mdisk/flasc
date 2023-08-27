#ifndef SERVER_H
#define SERVER_H
#include "flasc.h"

http_response text_response(char* s);
http_response file_response(char* file_name);

typedef enum RouteType { HANDLER_ROUTE, STATIC_DIR } RouteType;

/* IDEA: Add middleware support (something ran before actual handler)
http_res (*middleware)(http_req req, http_res(*next)(http_req req));
*/
// IDEA: Error handler function
// http_response (*error_handler)(http_request req, int error_code);
typedef struct router {
  struct {
    char* path;
    http_response (*handler)(http_request req);
    RouteType type;
    char* base_dir;  // This is only filled when type is STATIC_DIR.
    // TODO: Routes should not have limit, dynamic allowcation and keep counter
    // instead
  } routes[MAX_ROUTES];
  int num_routes;
} router;

int init_server(char* port, router r);
int init_router(router* r);
void register_route(router* r, char* path,
                    http_response (*handler)(http_request req));

void register_static_dir(router* r, char* path, char* base_dir);
#endif