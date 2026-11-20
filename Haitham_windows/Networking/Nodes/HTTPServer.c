#include "HTTPServer.h"
#include "../../Systems/ThreadPool.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define REQUEST_CAP     30000
#define PATH_CAP        2048
#define WORKER_THREADS  8
#define RECV_TIMEOUT_MS 10000   /* a silent client must not pin a worker thread forever */

#define ERR_400 "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
#define ERR_413 "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"

struct Route {
    int methods[9];
    int method_count;
    char *(*route_function)(struct HTTPServer *, struct HTTPRequest *);
};
struct ClientServer { SOCKET client; struct HTTPServer *server; };

static DWORD WINAPI client_worker(LPVOID arg);

static void register_routes(struct HTTPServer *s, char *(*fn)(struct HTTPServer *, struct HTTPRequest *),
                            char *uri, int n, ...)
{
    struct Route r = {0};
    va_list ap;
    if (!s || !fn || !uri || n < 1 || n > 9) return;
    r.method_count = n;
    r.route_function = fn;
    va_start(ap, n);
    for (int i = 0; i < n; i++) r.methods[i] = va_arg(ap, int);
    va_end(ap);
    s->routes.insert(&s->routes, uri, (unsigned long)strlen(uri) + 1, &r, sizeof(r));
}

struct HTTPServer http_server_constructor(void)
{
    struct HTTPServer h = {0};
    h.server = server_constructor(AF_INET, SOCK_STREAM, IPPROTO_TCP, INADDR_ANY, 8080, 128);
    h.routes = dictionary_constructor(compare_string_keys);
    h.register_routes = register_routes;
    h.launch = launch;
    return h;
}

static int method_allowed(const struct Route *r, const char *m)
{
    static const char *names[] = { "CONNECT", "DELETE", "GET", "HEAD", "OPTIONS", "PATCH", "POST", "PUT", "TRACE" };
    for (int i = 0; i < r->method_count; i++)
        if (r->methods[i] >= 0 && r->methods[i] < 9 && !strcmp(m, names[r->methods[i]])) return 1;
    return 0;
}

static int send_all(SOCKET s, const char *b, size_t n)
{
    size_t sent = 0;
    while (sent < n) {
        int k = send(s, b + sent, (int)(n - sent), 0);
        if (k == SOCKET_ERROR || k == 0) return 0;
        sent += (size_t)k;
    }
    return 1;
}

static void send_str(SOCKET s, const char *str) { send_all(s, str, strlen(str)); }

static void handle_request(SOCKET sock, struct HTTPServer *s, char *raw)
{
    static const char *err400 = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    static const char *err404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    static const char *err405 = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    static const char *err414 = "HTTP/1.1 414 URI Too Long\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";

    struct HTTPRequest req = http_request_constructor(raw);
    const char *uri = req.request_line.search(&req.request_line, "uri", 4);
    const char *method = req.request_line.search(&req.request_line, "method", 7);

    if (!uri || !method) {
        send_str(sock, err400);
    } else {
        /* Route on the path only: "/?a=b" and "/#x" must still match the "/" route. */
        char path[PATH_CAP];
        size_t len = strcspn(uri, "?#");
        if (len >= sizeof(path)) {
            send_str(sock, err414);
        } else {
            struct Route *r;
            memcpy(path, uri, len);
            path[len] = '\0';
            r = s->routes.search(&s->routes, path, (unsigned long)len + 1);
            if (!r) {
                send_str(sock, err404);
            } else if (!method_allowed(r, method)) {
                send_str(sock, err405);
            } else {
                char *response = r->route_function(s, &req);
                if (response) { send_str(sock, response); free(response); }   /* routes return malloc()ed text */
            }
        }
    }
    http_request_destructor(&req);
}

/* Returns the Content-Length of the header block h[0..n), 0 if absent, -1 if invalid/too large. */
static long header_content_length(const char *h, size_t n)
{
    static const char key[] = "content-length:";
    const char *end = h + n;
    const char *line = memchr(h, '\n', n);          /* skip the request line */
    long value = 0;
    int found = 0;

    while (line && line + 1 < end) {
        const char *p = line + 1;
        size_t i = 0;
        while (key[i] && p + i < end && tolower((unsigned char)p[i]) == key[i]) i++;
        if (!key[i]) {
            const char *v = p + i;
            long parsed = 0;
            int digits = 0;
            while (v < end && (*v == ' ' || *v == '\t')) v++;
            while (v < end && *v >= '0' && *v <= '9') {
                parsed = parsed * 10 + (*v - '0');
                if (parsed > 1000000) return -1;
                v++; digits++;
            }
            while (v < end && (*v == ' ' || *v == '\t' || *v == '\r')) v++;
            if (!digits || (v < end && *v != '\n')) return -1;
            if (found && parsed != value) return -1;   /* conflicting duplicate headers */
            value = parsed;
            found = 1;
        }
        line = memchr(p, '\n', (size_t)(end - p));
    }
    return value;
}

static DWORD WINAPI client_worker(LPVOID arg)
{
    struct ClientServer *c = arg;
    SOCKET sock;
    struct HTTPServer *s;
    char buf[REQUEST_CAP];
    int used = 0;
    size_t hdr_len = 0;                         /* header block length incl. the blank line */
    DWORD timeout = RECV_TIMEOUT_MS;

    if (!c) return 0;
    sock = c->client;
    s = c->server;
    free(c);

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

    while (used < (int)sizeof(buf) - 1) {
        char *end;
        int n = recv(sock, buf + used, (int)sizeof(buf) - 1 - used, 0);
        if (n <= 0) break;                      /* closed, error, or timeout */
        used += n;
        buf[used] = '\0';
        end = strstr(buf, "\r\n\r\n");
        if (end) { hdr_len = (size_t)(end - buf) + 4; break; }
    }

    if (used > 0) {
        long clen = hdr_len ? header_content_length(buf, hdr_len) : -1;
        if (!hdr_len || clen < 0) {
            send_str(sock, ERR_400);
        } else if ((size_t)clen > sizeof(buf) - 1 - hdr_len) {
            send_str(sock, ERR_413);
        } else {
            int ok = 1;
            while ((size_t)used < hdr_len + (size_t)clen) {     /* read the rest of the body */
                int n = recv(sock, buf + used, (int)(sizeof(buf) - 1 - (size_t)used), 0);
                if (n <= 0) { ok = 0; break; }
                used += n;
            }
            if (!ok) {
                send_str(sock, ERR_400);
            } else {
                buf[hdr_len + (size_t)clen] = '\0';             /* ignore anything past the body */
                handle_request(sock, s, buf);
            }
        }
    }

    /* Half-close first so the peer receives everything we sent instead of a TCP reset. */
    shutdown(sock, SD_SEND);
    closesocket(sock);
    return 0;
}

void launch(struct HTTPServer *s)
{
    struct ThreadPool pool;
    if (!s) return;

    pool = thread_pool_constructor(WORKER_THREADS);
    if (!pool.implementation) {
        fprintf(stderr, "thread pool creation failed\n");
        return;
    }

    while (!s->stopping) {
        struct sockaddr_in peer;
        int peer_len = (int)sizeof(peer);
        struct ClientServer *c;
        SOCKET client = accept(s->server.socket, (struct sockaddr *)&peer, &peer_len);

        if (client == INVALID_SOCKET) {
            int err = WSAGetLastError();
            if (s->stopping) break;
            /* The listening socket is gone/unusable: retrying would spin forever. */
            if (err == WSAEINTR || err == WSAENOTSOCK || err == WSAEINVAL) break;
            fprintf(stderr, "accept failed: %d\n", err);
            Sleep(50);                          /* transient (e.g. WSAEMFILE): avoid a hot loop */
            continue;
        }

        c = malloc(sizeof(*c));
        if (!c) { closesocket(client); continue; }
        c->client = client;
        c->server = s;
        if (!thread_pool_add_work(&pool, thread_job_constructor(client_worker, c))) {
            free(c);
            closesocket(client);
        }
    }

    thread_pool_destructor(&pool);              /* drains queued connections, joins workers */
}

void http_server_stop(struct HTTPServer *s)
{
    SOCKET listener;
    if (!s) return;
    InterlockedExchange(&s->stopping, 1);
    listener = s->server.socket;
    s->server.socket = INVALID_SOCKET;
    if (listener != INVALID_SOCKET) closesocket(listener);   /* unblocks accept() in launch() */
}

void http_server_destructor(struct HTTPServer *s)
{
    if (!s) return;
    dictionary_destructor(&s->routes);
    server_destructor(&s->server);
}

char *render_template(int count, ...)
{
    size_t cap = 1024, len = 0;
    char *out = malloc(cap);
    va_list ap;
    if (!out) return NULL;
    out[0] = '\0';

    va_start(ap, count);
    for (int i = 0; i < count; i++) {
        const char *path = va_arg(ap, const char *);
        FILE *f = path ? fopen(path, "rb") : NULL;
        size_t got;
        if (!f) continue;
        do {
            if (len + 4096 + 1 > cap) {
                char *t;
                while (len + 4096 + 1 > cap) cap *= 2;
                t = realloc(out, cap);
                if (!t) { fclose(f); free(out); va_end(ap); return NULL; }
                out = t;
            }
            got = fread(out + len, 1, 4096, f);
            len += got;
        } while (got > 0);
        fclose(f);
    }
    va_end(ap);
    out[len] = '\0';
    return out;
}
