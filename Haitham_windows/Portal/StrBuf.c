#include "StrBuf.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static int reserve(struct StrBuf *b, size_t extra)
{
    size_t need = b->len + extra + 1;
    if (b->failed) return 0;
    if (need > b->cap) {
        size_t cap = b->cap ? b->cap : 1024;
        char *p;
        while (cap < need) cap *= 2;
        p = realloc(b->data, cap);
        if (!p) { b->failed = 1; return 0; }
        b->data = p;
        b->cap = cap;
    }
    return 1;
}

void sb_init(struct StrBuf *b) { b->data = NULL; b->len = b->cap = 0; b->failed = 0; }

void sb_appendn(struct StrBuf *b, const char *s, size_t n)
{
    if (!s || !reserve(b, n)) return;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

void sb_append(struct StrBuf *b, const char *s) { if (s) sb_appendn(b, s, strlen(s)); }

void sb_appendf(struct StrBuf *b, const char *fmt, ...)
{
    size_t size = 256;
    if (b->failed) return;
    for (;;) {
        va_list ap;
        int n;
        char *tmp = malloc(size);
        if (!tmp) { b->failed = 1; return; }
        va_start(ap, fmt);
        n = vsnprintf(tmp, size, fmt, ap);
        va_end(ap);
        if (n >= 0 && (size_t)n < size) { sb_appendn(b, tmp, (size_t)n); free(tmp); return; }
        free(tmp);
        size = n >= 0 ? (size_t)n + 1 : size * 2;   /* old CRTs return -1 on truncation */
        if (size > ((size_t)1 << 20)) { b->failed = 1; return; }
    }
}

void sb_append_escaped(struct StrBuf *b, const char *s)
{
    if (!s) return;
    for (; *s; s++) {
        switch (*s) {
        case '&':  sb_append(b, "&amp;");  break;
        case '<':  sb_append(b, "&lt;");   break;
        case '>':  sb_append(b, "&gt;");   break;
        case '"':  sb_append(b, "&quot;"); break;
        case '\'': sb_append(b, "&#39;");  break;
        default:   sb_appendn(b, s, 1);    break;
        }
    }
}

char *sb_take(struct StrBuf *b)
{
    char *out;
    if (!b->failed && !b->data && reserve(b, 0)) b->data[0] = '\0';
    out = b->failed ? NULL : b->data;
    if (b->failed) free(b->data);
    sb_init(b);
    return out;
}

void sb_free(struct StrBuf *b) { free(b->data); sb_init(b); }
