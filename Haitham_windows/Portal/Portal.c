#include "Portal.h"
#include "Pages.h"
#include "Session.h"
#include "StrBuf.h"
#include "StudentData.h"
#include <stdlib.h>
#include <string.h>

/* Everything except `style-src 'self'` is locked down; the pages use no inline scripts or styles. */
#define SECURITY_HEADERS \
    "Cache-Control: no-store\r\n" \
    "X-Content-Type-Options: nosniff\r\n" \
    "X-Frame-Options: DENY\r\n" \
    "Referrer-Policy: no-referrer\r\n" \
    "Content-Security-Policy: default-src 'self'; style-src 'self'; form-action 'self'; " \
    "frame-ancestors 'none'; base-uri 'none'\r\n"

#define COOKIE_ATTRS "Path=/; HttpOnly; SameSite=Strict"

/* ---- Response helpers --------------------------------------------------------------------- */

static char *build_response(int status, const char *reason, const char *content_type,
                            const char *extra_headers, const char *body, size_t body_len)
{
    struct StrBuf b;
    sb_init(&b);
    sb_appendf(&b, "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %u\r\nConnection: close\r\n",
               status, reason, content_type, (unsigned)body_len);
    sb_append(&b, SECURITY_HEADERS);
    sb_append(&b, extra_headers);
    sb_append(&b, "\r\n");
    sb_appendn(&b, body, body_len);
    return sb_take(&b);
}

static char *server_error(void)
{
    static const char msg[] = "Internal server error";
    return build_response(500, "Internal Server Error", "text/plain; charset=utf-8", NULL, msg, sizeof(msg) - 1);
}

/* Takes ownership of `body` (a malloc()ed string, or NULL if page generation failed). */
static char *html_response(int status, const char *reason, char *body, const char *extra_headers)
{
    char *response;
    if (!body) return server_error();
    response = build_response(status, reason, "text/html; charset=utf-8", extra_headers, body, strlen(body));
    free(body);
    return response ? response : server_error();
}

static char *redirect(const char *location, const char *extra_headers)
{
    struct StrBuf h;
    char *response;
    sb_init(&h);
    sb_appendf(&h, "Location: %s\r\n", location);
    sb_append(&h, extra_headers);
    response = build_response(303, "See Other", "text/plain; charset=utf-8", h.data, "", 0);
    sb_free(&h);
    return response ? response : server_error();
}

/* ---- Request helpers ---------------------------------------------------------------------- */

static const char *field(struct Dictionary *d, const char *key)
{
    return d->search(d, (void *)key, (unsigned long)strlen(key) + 1);
}

/* Copies the value of cookie `name` from the Cookie header ("a=1; session=abc; b=2") into out. */
static int cookie_get(struct HTTPRequest *req, const char *name, char *out, size_t out_size)
{
    const char *p = field(&req->header_fields, "Cookie");
    size_t name_len = strlen(name);
    if (!p) return 0;

    while (*p) {
        const char *end;
        while (*p == ' ' || *p == ';') p++;
        end = p + strcspn(p, ";");
        if ((size_t)(end - p) > name_len && strncmp(p, name, name_len) == 0 && p[name_len] == '=') {
            size_t len = (size_t)(end - (p + name_len + 1));
            if (len == 0 || len >= out_size) return 0;
            memcpy(out, p + name_len + 1, len);
            out[len] = '\0';
            return 1;
        }
        p = end;
    }
    return 0;
}

static int cookie_token(struct HTTPRequest *req, char token[SESSION_TOKEN_SIZE])
{
    return cookie_get(req, "session", token, SESSION_TOKEN_SIZE);
}

static const struct Student *current_student(struct HTTPRequest *req)
{
    char token[SESSION_TOKEN_SIZE];
    return cookie_token(req, token) ? session_lookup(token) : NULL;
}

/* ---- Route handlers ----------------------------------------------------------------------- */

static char *handle_root(struct HTTPServer *srv, struct HTTPRequest *req)
{
    (void)srv;
    return redirect(current_student(req) ? "/home" : "/login", NULL);
}

static char *login_submit(struct HTTPRequest *req)
{
    const char *name = field(&req->body, "name");
    const char *password = field(&req->body, "password");
    const struct Student *student;
    char token[SESSION_TOKEN_SIZE];
    char cookie[160];
    const char *error;

    if (!name || !*name || !password || !*password) {
        error = "Please enter both your name and password.";
    } else if (!(student = student_authenticate(name, password))) {
        error = "Incorrect name or password.";            /* same message for either mistake */
    } else if (!session_create(student, token)) {
        return server_error();
    } else {
        snprintf(cookie, sizeof(cookie), "Set-Cookie: session=%s; " COOKIE_ATTRS "; Max-Age=%d\r\n",
                 token, SESSION_TTL_SECONDS);
        return redirect("/home", cookie);
    }
    return html_response(401, "Unauthorized", page_login(error), NULL);
}

static char *handle_login(struct HTTPServer *srv, struct HTTPRequest *req)
{
    const char *method = field(&req->request_line, "method");
    (void)srv;
    if (method && strcmp(method, "POST") == 0) return login_submit(req);
    if (current_student(req)) return redirect("/home", NULL);
    return html_response(200, "OK", page_login(NULL), NULL);
}

static char *handle_home(struct HTTPServer *srv, struct HTTPRequest *req)
{
    const struct Student *student = current_student(req);
    (void)srv;
    if (!student) return redirect("/login", NULL);
    return html_response(200, "OK", page_home(student), NULL);
}

static char *handle_logout(struct HTTPServer *srv, struct HTTPRequest *req)
{
    char token[SESSION_TOKEN_SIZE];
    (void)srv;
    if (cookie_token(req, token)) session_destroy(token);
    return redirect("/login", "Set-Cookie: session=; " COOKIE_ATTRS "; Max-Age=0\r\n");
}

static char *handle_style(struct HTTPServer *srv, struct HTTPRequest *req)
{
    char *css = page_stylesheet();
    char *response;
    (void)srv; (void)req;
    if (!css) return server_error();
    response = build_response(200, "OK", "text/css; charset=utf-8", NULL, css, strlen(css));
    free(css);
    return response ? response : server_error();
}

/* ---- Registration ------------------------------------------------------------------------- */

int portal_register(struct HTTPServer *s)
{
    if (!s || !session_store_init()) return 0;
    s->register_routes(s, handle_root,   "/",          1, HTTP_GET);
    s->register_routes(s, handle_login,  "/login",     2, HTTP_GET, HTTP_POST);
    s->register_routes(s, handle_home,   "/home",      1, HTTP_GET);
    s->register_routes(s, handle_logout, "/logout",    1, HTTP_POST);
    s->register_routes(s, handle_style,  "/style.css", 1, HTTP_GET);
    return 1;
}

void portal_shutdown(void) { session_store_shutdown(); }
