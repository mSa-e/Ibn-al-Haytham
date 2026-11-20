#ifndef PORTAL_STRBUF_H
#define PORTAL_STRBUF_H
#include <stddef.h>
#include <stdio.h>

/* Growable string used to assemble HTML and HTTP responses. Allocation failure is remembered
 * in `failed`; sb_take() then returns NULL, so callers only need to check once at the end. */
struct StrBuf { char *data; size_t len, cap; int failed; };

#if defined(__MINGW32__) && defined(__MINGW_PRINTF_FORMAT)
#define SB_PRINTF __attribute__((format(__MINGW_PRINTF_FORMAT, 2, 3)))
#elif defined(__GNUC__)
#define SB_PRINTF __attribute__((format(printf, 2, 3)))
#else
#define SB_PRINTF
#endif

void sb_init(struct StrBuf *b);
void sb_append(struct StrBuf *b, const char *s);
void sb_appendn(struct StrBuf *b, const char *s, size_t n);
void sb_appendf(struct StrBuf *b, const char *fmt, ...) SB_PRINTF;
void sb_append_escaped(struct StrBuf *b, const char *s);   /* HTML-escapes & < > " ' */
char *sb_take(struct StrBuf *b);                           /* malloc()ed string, or NULL on failure; resets b */
void sb_free(struct StrBuf *b);
#endif
