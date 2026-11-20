#ifndef Haitham_HTTPSERVER_H
#define Haitham_HTTPSERVER_H
#include "Server.h"
#include "../Protocols/HTTPRequest.h"

struct HTTPServer {
    struct Server server;
    struct Dictionary routes;
    volatile LONG stopping;                     /* set by http_server_stop() */
    void (*register_routes)(struct HTTPServer *, char *(*)(struct HTTPServer *, struct HTTPRequest *), char *, int, ...);
    void (*launch)(struct HTTPServer *);
};

/* Prefixed on purpose: <winnt.h> defines DELETE as a macro (0x00010000L), so the original
 * unprefixed `DELETE` enumerator could not compile on Windows. */
enum HTTPMethods {
    HTTP_CONNECT, HTTP_DELETE, HTTP_GET, HTTP_HEAD, HTTP_OPTIONS,
    HTTP_PATCH, HTTP_POST, HTTP_PUT, HTTP_TRACE
};

/* Route handlers: char *fn(struct HTTPServer *, struct HTTPRequest *).
 * They must return a complete HTTP response as a malloc()ed string (or NULL to send nothing);
 * the server sends it and then free()s it. */
struct HTTPServer http_server_constructor(void);
void launch(struct HTTPServer *server);         /* blocks until http_server_stop() is called */
void http_server_stop(struct HTTPServer *server);   /* thread-safe; safe from a console-control handler */
void http_server_destructor(struct HTTPServer *server);
char *render_template(int num_templates, ...);  /* returns a malloc()ed string the caller must free() */
#endif
