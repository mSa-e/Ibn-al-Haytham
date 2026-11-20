#include "ThreadPool.h"
#include <stdlib.h>

typedef struct PoolState {
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE ready;
    int stopping;
    struct Queue queue;
} PoolState;

static DWORD WINAPI worker(LPVOID p)
{
    PoolState *s = p;
    for (;;) {
        struct ThreadJob job = {0};
        struct ThreadJob *head;

        EnterCriticalSection(&s->lock);
        while (!s->stopping && !s->queue.peek(&s->queue))
            SleepConditionVariableCS(&s->ready, &s->lock, INFINITE);

        head = s->queue.peek(&s->queue);
        if (!head) {                    /* stopping and nothing left to drain */
            LeaveCriticalSection(&s->lock);
            break;
        }
        job = *head;
        s->queue.pop(&s->queue);
        LeaveCriticalSection(&s->lock);

        if (job.job) job.job(job.arg);
    }
    return 0;
}

static int add_work_impl(struct ThreadPool *p, struct ThreadJob j)
{
    PoolState *s;
    int queued = 0;
    if (!p || !p->implementation || !j.job) return 0;
    s = p->implementation;

    EnterCriticalSection(&s->lock);
    if (!s->stopping) {
        int before = s->queue.list.length;
        s->queue.push(&s->queue, &j, sizeof(j));
        queued = s->queue.list.length > before;     /* push() is void; detect malloc failure */
    }
    LeaveCriticalSection(&s->lock);

    if (queued) WakeConditionVariable(&s->ready);
    return queued;
}

struct ThreadJob thread_job_constructor(DWORD (WINAPI *fn)(LPVOID), LPVOID arg)
{
    struct ThreadJob j = { fn, arg };
    return j;
}

struct ThreadPool thread_pool_constructor(int n)
{
    struct ThreadPool p = {0};
    PoolState *s;

    if (n < 1) n = 1;
    p.num_threads = n;
    p.work = queue_constructor();
    p.add_work = add_work_impl;

    s = calloc(1, sizeof(*s));
    if (!s) { p.num_threads = 0; return p; }
    s->queue = queue_constructor();
    InitializeCriticalSection(&s->lock);
    InitializeConditionVariable(&s->ready);

    p.pool = calloc((size_t)n, sizeof(HANDLE));
    if (!p.pool) {
        DeleteCriticalSection(&s->lock);
        free(s);
        p.num_threads = 0;
        return p;
    }
    p.implementation = s;
    p.active = 1;

    for (int i = 0; i < n; i++) {
        p.pool[i] = CreateThread(NULL, 0, worker, s, 0, NULL);
        if (!p.pool[i]) {
            p.num_threads = i;                  /* only join the threads that really started */
            thread_pool_destructor(&p);
            break;
        }
    }
    return p;
}

int thread_pool_add_work(struct ThreadPool *p, struct ThreadJob j)
{
    return add_work_impl(p, j);
}

void thread_pool_destructor(struct ThreadPool *p)
{
    PoolState *s;
    if (!p || !p->implementation) return;
    s = p->implementation;

    EnterCriticalSection(&s->lock);
    s->stopping = 1;
    WakeAllConditionVariable(&s->ready);
    LeaveCriticalSection(&s->lock);

    /* WaitForMultipleObjects() is capped at MAXIMUM_WAIT_OBJECTS (64) handles, so wait one by one. */
    for (int i = 0; i < p->num_threads; i++) {
        WaitForSingleObject(p->pool[i], INFINITE);
        CloseHandle(p->pool[i]);
    }
    free(p->pool);
    p->pool = NULL;
    p->num_threads = 0;

    queue_destructor(&s->queue);
    DeleteCriticalSection(&s->lock);
    free(s);
    p->implementation = NULL;
    p->active = 0;
}
