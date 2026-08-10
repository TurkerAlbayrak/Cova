#ifndef THREADPOOL_H
#define THREADPOOL_H

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef void (*ThreadFunc)(void *arg);

typedef struct ThreadTask {
    ThreadFunc func;
    void *arg;
    struct ThreadTask *next;
} ThreadTask;

typedef struct ThreadPool {
    ThreadTask *head;
    ThreadTask *tail;
    int count;
    int shutdown;
#ifdef _WIN32
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE notify;
    HANDLE *threads;
#else
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
#endif
    int thread_count;
} ThreadPool;

ThreadPool* threadpool_create(int num_threads);
void threadpool_add_task(ThreadPool *pool, ThreadFunc func, void *arg);
void threadpool_destroy(ThreadPool *pool);

#endif
