#include "HTTPRequest.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Re-entrant tokenizer: unlike strtok(), this is safe across worker threads. */
static char *next_token(char **cursor, const char *delimiters)
{
    char *start;
    char *end;
    if (!cursor || !*cursor) return NULL;
    start = *cursor;
    start += strspn(start, delimiters);
    if (*start == '\0') { *cursor = NULL; return NULL; }
    end = start + strcspn(start, delimiters);
    if (*end) { *end = '\0'; *cursor = end + 1; }
    else { *cursor = NULL; }
    return start;
}

static char *trim(char *s)
{
    char *end;
    while (*s && isspace((unsigned char)*s)) ++s;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return s;
}

/* HTTP header names are case-insensitive ("content-type" == "Content-Type"). */
static int compare_header_names(void *a, void *b)
{
    const unsigned char *x = a, *y = b;
    while (*x && *y) {
        int cx = tolower(*x), cy = tolower(*y);
        if (cx != cy) return cx - cy;
        x++; y++;
    }
    return tolower(*x) - tolower(*y);
}

static int starts_with_nocase(const char *s, const char *prefix)
{
    while (*prefix) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return 0;
        s++; prefix++;
    }
    return 1;
}

static int hex_value(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* In-place application/x-www-form-urlencoded decoding ('+' -> space, %XX -> byte). */
static void url_decode(char *s)
{
    char *out = s;
    while (*s) {
        if (*s == '+') { *out++ = ' '; s++; }
        else if (*s == '%' && hex_value((unsigned char)s[1]) >= 0 && hex_value((unsigned char)s[2]) >= 0) {
            *out++ = (char)(hex_value((unsigned char)s[1]) * 16 + hex_value((unsigned char)s[2]));
            s += 3;
        } else *out++ = *s++;
    }
    *out = '\0';
}

static void parse_form(struct Dictionary *d, char *body)
{
    char *cursor = body, *pair;
    while ((pair = next_token(&cursor, "&")) != NULL) {
        char *eq = strchr(pair, '=');
        char *key = pair, *value = "";
        if (eq) { *eq = '\0'; value = eq + 1; }
        url_decode(key);
        url_decode(value);
        if (*key) d->insert(d, key, (unsigned long)strlen(key) + 1, value, (unsigned long)strlen(value) + 1);
    }
}

struct HTTPRequest http_request_constructor(char *raw)
{
    struct HTTPRequest request = {0};
    char *copy, *cursor, *line;
    char *body = NULL;
    int is_form = 0;
    size_t length;

    request.request_line = dictionary_constructor(compare_string_keys);
    request.header_fields = dictionary_constructor(compare_header_names);
    request.body = dictionary_constructor(compare_string_keys);
    if (!raw) return request;

    length = strlen(raw);
    copy = malloc(length + 1);
    if (!copy) return request;
    memcpy(copy, raw, length + 1);

    {
        char *header_end = strstr(copy, "\r\n\r\n");
        if (header_end) { body = header_end + 4; *header_end = '\0'; }
    }
    cursor = copy;
    line = next_token(&cursor, "\r\n");
    if (line) {
        char *parts = line;
        char *method = next_token(&parts, " \t");
        char *uri = next_token(&parts, " \t");
        char *version = next_token(&parts, " \t");
        if (method) request.request_line.insert(&request.request_line, "method", 7, method, (unsigned long)strlen(method) + 1);
        if (uri) request.request_line.insert(&request.request_line, "uri", 4, uri, (unsigned long)strlen(uri) + 1);
        if (version) request.request_line.insert(&request.request_line, "http_version", 12, version, (unsigned long)strlen(version) + 1);
    }

    while ((line = next_token(&cursor, "\r\n")) != NULL) {
        char *colon;
        char *key;
        char *value;
        if (*line == '\0') break;
        colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';
        key = trim(line);
        value = trim(colon + 1);
        if (compare_header_names(key, "content-type") == 0 && starts_with_nocase(value, "application/x-www-form-urlencoded"))
            is_form = 1;
        if (*key) request.header_fields.insert(&request.header_fields, key, (unsigned long)strlen(key) + 1, value, (unsigned long)strlen(value) + 1);
    }

    if (is_form && body && *body) parse_form(&request.body, body);

    free(copy);
    return request;
}

void http_request_destructor(struct HTTPRequest *request)
{
    if (!request) return;
    dictionary_destructor(&request->request_line);
    dictionary_destructor(&request->header_fields);
    dictionary_destructor(&request->body);
}
