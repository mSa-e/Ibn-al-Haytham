#include "Server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void register_server_route(struct Server *s, char *(*fn)(void *), char *path)
{
    struct ServerRoute r;
    if (!s || !fn || !path) return;
    r.route_function = fn;
    s->routes.insert(&s->routes, path, (unsigned long)strlen(path) + 1, &r, sizeof(r));
}

static void fail(struct Server *s, const char *what)
{
    fprintf(stderr, "%s failed: %d\n", what, WSAGetLastError());
    if (s->socket != INVALID_SOCKET) closesocket(s->socket);
    dictionary_destructor(&s->routes);
    exit(EXIT_FAILURE);
}

struct Server server_constructor(int domain, int service, int protocol, u_long iface, int port, int backlog)
{
    struct Server s;
    memset(&s, 0, sizeof(s));
    s.socket = INVALID_SOCKET;
    s.domain = domain; s.service = service; s.protocol = protocol;
    s.iface = iface; s.port = port; s.backlog = backlog;

    s.address.sin_family = (ADDRESS_FAMILY)domain;
    s.address.sin_port = htons((u_short)port);
    s.address.sin_addr.s_addr = htonl(iface);

    s.routes = dictionary_constructor(compare_string_keys);
    s.register_routes = register_server_route;

    s.socket = socket(domain, service, protocol);
    if (s.socket == INVALID_SOCKET) fail(&s, "socket");

    /* On Windows SO_REUSEADDR lets another process hijack the port; the safe equivalent is
     * SO_EXCLUSIVEADDRUSE, which also makes a second bind() fail cleanly with WSAEADDRINUSE. */
    {
        BOOL excl = TRUE;
        setsockopt(s.socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char *)&excl, sizeof(excl));
    }

    if (bind(s.socket, (struct sockaddr *)&s.address, sizeof(s.address)) == SOCKET_ERROR) fail(&s, "bind");
    if (listen(s.socket, backlog) == SOCKET_ERROR) fail(&s, "listen");
    return s;
}

void server_destructor(struct Server *s)
{
    if (!s) return;
    if (s->socket != INVALID_SOCKET) {
        closesocket(s->socket);
        s->socket = INVALID_SOCKET;
    }
    dictionary_destructor(&s->routes);
}
