#ifndef SERVER_H
#define SERVER_H
#include "flasc.h"
void text_response(http_request req, char *s);
int init_server(char *port);

#endif