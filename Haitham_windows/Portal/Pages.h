#ifndef PORTAL_PAGES_H
#define PORTAL_PAGES_H

struct Student;

/* Each function returns a malloc()ed string (NULL on out-of-memory) that the caller must free(). */
char *page_login(const char *error_message);        /* error_message may be NULL */
char *page_home(const struct Student *student);
char *page_stylesheet(void);
#endif
