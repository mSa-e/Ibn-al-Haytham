#ifndef Haitham_THREADPOOL_H
#define Haitham_THREADPOOL_H
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "../DataStructures/Lists/Queue.h"

struct ThreadJob { DWORD (WINAPI *job)(LPVOID); LPVOID arg; };

struct ThreadPool {
    int num_threads;
    int active;
    struct Queue work;          /* unused; kept for source compatibility */
    HANDLE *pool;
    void *implementation;       /* heap-allocated PoolState shared with the workers */
    /* Returns 1 if the job was queued, 0 if it was rejected (pool stopping / out of memory). */
    int (*add_work)(struct ThreadPool *, struct ThreadJob);
};

struct ThreadPool thread_pool_constructor(int num_threads);
struct ThreadJob thread_job_constructor(DWORD (WINAPI *job)(LPVOID), LPVOID arg);
int thread_pool_add_work(struct ThreadPool *pool, struct ThreadJob job);
void thread_pool_destructor(struct ThreadPool *pool);
#endif
