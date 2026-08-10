#include "threadpool.h"
#include "memtrack.h"
#include <stdlib.h>

#ifdef _WIN32
DWORD WINAPI threadpool_worker(LPVOID arg) {
#else
void* threadpool_worker(void *arg) {
#endif
    ThreadPool *pool = (ThreadPool*)arg;
    while (1) {
        ThreadTask *task = NULL;
#ifdef _WIN32
        EnterCriticalSection(&pool->lock);
        while (pool->count == 0 && !pool->shutdown) {
            SleepConditionVariableCS(&pool->notify, &pool->lock, INFINITE);
        }
        if (pool->shutdown) {
            LeaveCriticalSection(&pool->lock);
            break;
        }
        task = pool->head;
        if (task) {
            pool->head = task->next;
            if (pool->head == NULL) pool->tail = NULL;
            pool->count--;
        }
        LeaveCriticalSection(&pool->lock);
#else
        pthread_mutex_lock(&pool->lock);
        while (pool->count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }
        task = pool->head;
        if (task) {
            pool->head = task->next;
            if (pool->head == NULL) pool->tail = NULL;
            pool->count--;
        }
        pthread_mutex_unlock(&pool->lock);
#endif
        if (task) {
            task->func(task->arg);
            cova_free(task);
        }
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

ThreadPool* threadpool_create(int num_threads) {
    ThreadPool *pool = (ThreadPool*)cova_malloc(sizeof(ThreadPool));
    pool->head = NULL;
    pool->tail = NULL;
    pool->count = 0;
    pool->shutdown = 0;
    pool->thread_count = num_threads;

#ifdef _WIN32
    InitializeCriticalSection(&pool->lock);
    InitializeConditionVariable(&pool->notify);
    pool->threads = (HANDLE*)cova_malloc(sizeof(HANDLE) * num_threads);
    for (int i = 0; i < num_threads; i++) {
        pool->threads[i] = CreateThread(NULL, 0, threadpool_worker, pool, 0, NULL);
    }
#else
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->notify, NULL);
    pool->threads = (pthread_t*)cova_malloc(sizeof(pthread_t) * num_threads);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, threadpool_worker, pool);
    }
#endif
    return pool;
}

void threadpool_add_task(ThreadPool *pool, ThreadFunc func, void *arg) {
    if (!pool) return;
    ThreadTask *task = (ThreadTask*)cova_malloc(sizeof(ThreadTask));
    task->func = func;
    task->arg = arg;
    task->next = NULL;

#ifdef _WIN32
    EnterCriticalSection(&pool->lock);
    if (pool->tail) {
        pool->tail->next = task;
        pool->tail = task;
    } else {
        pool->head = task;
        pool->tail = task;
    }
    pool->count++;
    WakeConditionVariable(&pool->notify);
    LeaveCriticalSection(&pool->lock);
#else
    pthread_mutex_lock(&pool->lock);
    if (pool->tail) {
        pool->tail->next = task;
        pool->tail = task;
    } else {
        pool->head = task;
        pool->tail = task;
    }
    pool->count++;
    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
#endif
}

void threadpool_destroy(ThreadPool *pool) {
    if (!pool) return;
    
#ifdef _WIN32
    EnterCriticalSection(&pool->lock);
    pool->shutdown = 1;
    WakeAllConditionVariable(&pool->notify);
    LeaveCriticalSection(&pool->lock);
    
    for (int i = 0; i < pool->thread_count; i++) {
        WaitForSingleObject(pool->threads[i], INFINITE);
        CloseHandle(pool->threads[i]);
    }
    DeleteCriticalSection(&pool->lock);
#else
    pthread_mutex_lock(&pool->lock);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
    
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
#endif

    // V21: Kuyrukta kalan ve islenmeyen gorevleri (ve argumanlarini) temizle
    ThreadTask *current = pool->head;
    while (current) {
        ThreadTask *next = current->next;
        if (current->arg) {
            cova_free(current->arg);
        }
        cova_free(current);
        current = next;
    }

    cova_free(pool->threads);
    cova_free(pool);
}
