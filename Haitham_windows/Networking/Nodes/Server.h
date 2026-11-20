#ifndef Haitham_SERVER_H
#define Haitham_SERVER_H
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "../../DataStructures/Dictionary/Dictionary.h"

/* NOTE: the field is called `iface`, not `interface`: <objbase.h> does `#define interface struct`,
 * which breaks any header that uses that identifier once windows.h is included without
 * WIN32_LEAN_AND_MEAN. */
struct Server {
    int domain, service, protocol;
    u_long iface;
    int port, backlog;
    struct sockaddr_in address;
    SOCKET socket;
    struct Dictionary routes;
    void (*register_routes)(struct Server *, char *(*)(void *), char *);
};

struct ServerRoute { char *(*route_function)(void *arg); };

struct Server server_constructor(int domain, int service, int protocol, u_long iface, int port, int backlog);
void server_destructor(struct Server *server);
#endif
