#ifndef PORTAL_SESSION_H
#define PORTAL_SESSION_H

#define SESSION_TOKEN_SIZE  33          /* 32 hex chars (128 random bits) + NUL */
#define SESSION_TTL_SECONDS 1800        /* 30 minutes, renewed on every authenticated request */

struct Student;

int  session_store_init(void);          /* call once from the main thread before launch() */
void session_store_shutdown(void);
int  session_create(const struct Student *student, char token_out[SESSION_TOKEN_SIZE]);   /* 1 on success */
const struct Student *session_lookup(const char *token);   /* NULL if unknown or expired */
void session_destroy(const char *token);
#endif
