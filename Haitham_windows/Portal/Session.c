#define _CRT_RAND_S                     /* enables rand_s(), the CRT's cryptographic RNG */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Session.h"

#define MAX_SESSIONS 64

struct Session {
    int used;
    char token[SESSION_TOKEN_SIZE];
    const struct Student *student;
    ULONGLONG expires;                  /* GetTickCount64() milliseconds */
};

static struct Session g_sessions[MAX_SESSIONS];
static CRITICAL_SECTION g_lock;
static int g_ready;

int session_store_init(void)
{
    if (g_ready) return 1;
    memset(g_sessions, 0, sizeof(g_sessions));
    InitializeCriticalSection(&g_lock);
    g_ready = 1;
    return 1;
}

void session_store_shutdown(void)
{
    if (!g_ready) return;
    g_ready = 0;
    DeleteCriticalSection(&g_lock);
    memset(g_sessions, 0, sizeof(g_sessions));
}

static int make_token(char out[SESSION_TOKEN_SIZE])
{
    for (int i = 0; i < 4; i++) {
        unsigned int r;
        if (rand_s(&r) != 0) return 0;
        snprintf(out + i * 8, 9, "%08x", r);
    }
    return 1;
}

int session_create(const struct Student *student, char token_out[SESSION_TOKEN_SIZE])
{
    char token[SESSION_TOKEN_SIZE];
    struct Session *slot = NULL;
    ULONGLONG now;
    if (!g_ready || !student || !make_token(token)) return 0;

    EnterCriticalSection(&g_lock);
    now = GetTickCount64();
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!g_sessions[i].used || g_sessions[i].expires <= now) { slot = &g_sessions[i]; break; }
    }
    if (!slot) {                                /* table full of live sessions: evict the oldest */
        slot = &g_sessions[0];
        for (int i = 1; i < MAX_SESSIONS; i++)
            if (g_sessions[i].expires < slot->expires) slot = &g_sessions[i];
    }
    slot->used = 1;
    memcpy(slot->token, token, sizeof(token));
    slot->student = student;
    slot->expires = now + (ULONGLONG)SESSION_TTL_SECONDS * 1000u;
    LeaveCriticalSection(&g_lock);

    memcpy(token_out, token, sizeof(token));
    return 1;
}

const struct Student *session_lookup(const char *token)
{
    const struct Student *found = NULL;
    ULONGLONG now;
    if (!g_ready || !token || !*token) return NULL;

    EnterCriticalSection(&g_lock);
    now = GetTickCount64();
    for (int i = 0; i < MAX_SESSIONS; i++) {
        struct Session *s = &g_sessions[i];
        if (!s->used) continue;
        if (s->expires <= now) { memset(s, 0, sizeof(*s)); continue; }
        if (strcmp(s->token, token) == 0) {
            s->expires = now + (ULONGLONG)SESSION_TTL_SECONDS * 1000u;
            found = s->student;
            break;
        }
    }
    LeaveCriticalSection(&g_lock);
    return found;
}

void session_destroy(const char *token)
{
    if (!g_ready || !token || !*token) return;
    EnterCriticalSection(&g_lock);
    for (int i = 0; i < MAX_SESSIONS; i++)
        if (g_sessions[i].used && strcmp(g_sessions[i].token, token) == 0) {
            memset(&g_sessions[i], 0, sizeof(g_sessions[i]));
            break;
        }
    LeaveCriticalSection(&g_lock);
}
