#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>

#include "common.h"

#include "mutex.h"
#include "taslock.h"
#include "tiketlock.h"
#include "arraylock.h"
#include "clhlock.h"
#include "mcslock.h"

using namespace std;

static int64_t counter    = 0;
static int64_t n          = 0;
static int64_t loop_count = 0;

static MutexLock mutex_lock;
static TASLock   tas_lock;
static TiketLock tiket_lock;
static ArrayLock array_lock;
static CLHLock   clh_lock;
static MCSLock   mcs_lock;

struct param
{
  void *lock;
  int64_t tid;
};

void adder(param *args)
{
  for (int64_t i = 0; i < loop_count; ++i) {
    BaseLockGuard guard(*(BaseLock*)args->lock);
    counter += 1;
  }
}

void adder_arraylock(param *args)
{
  for (int64_t i = 0; i < loop_count; ++i) {
    ArrayLockGuard guard(*(ArrayLock*)args->lock, args->tid);
    counter += 1;
  }
}

void adder_listlock(param *args)
{
  for (int64_t i = 0; i < loop_count; ++i) {
    ListLockGuard guard(*(ListLock*)args->lock);
    counter += 1;

    // if (10 * i % loop_count == 0) {
    //   printf("No. %5ld: %3ld%%\n", args->tid, 100 * i / loop_count);
    // }
  }
}

int main(int argc, char *argv[])
{
  pthread_t threads[MAX_THREAD_NUM];
  param args[MAX_THREAD_NUM];

  /* Check arguments to program*/
  if(argc != 3) {
      fprintf(stderr, "USAGE: %s <threads> <loopcount>\n", argv[0]);
      exit(1);
  }

  /* Parse argument */
  n          = min(atol(argv[1]), MAX_THREAD_NUM);
  loop_count = atol(argv[2]); /* Don't bother with format checking */

  /* Start the threads */
  for (int64_t i = 0; i < n; ++i) {
    args[i].lock = &mcs_lock;
    args[i].tid = i;

    pthread_create(&threads[i], NULL, (void* (*)(void*))adder_listlock, &args[i]);
  }

  for (int64_t i = 0; i < n; ++i) {
    pthread_join(threads[i], NULL);
  }

  if (loop_count * n != counter) {
    printf("race detected, counter: %ld != total: %ld\n", counter, loop_count * n);
  } else {
    printf("works fine!\n");
  }

  return 0;
}
