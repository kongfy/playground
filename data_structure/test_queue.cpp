#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <algorithm>

#include "common.h"
#include "queue.h"

using namespace std;

static int64_t n          = 0;
static int64_t loop_count = 0;

static Queue<int64_t> queue;

struct value {
  int64_t val;
} CACHE_ALIGNED counters[MAX_THREAD_NUM];

void worker(int64_t &counter)
{
  int64_t t;
  for (int64_t i = 0; i < loop_count; ++i) {
    queue.enqueue(i);
    if (queue.dequeue(t)) {
      counter += t;
    }
  }
}
#include <signal.h>
#include <unistd.h>
void segfaulthandler(int parameter)
{
  char gcore[50];
  sprintf(gcore, "gcore %u", getpid());
  system(gcore);
  printf("done\n");

  exit(1);
}

int main(int argc, char *argv[])
{
  signal(SIGSEGV, segfaulthandler);

  pthread_t threads[MAX_THREAD_NUM];

  /* Check arguments to program*/
  if(argc != 3) {
      fprintf(stderr, "USAGE: %s <threads> <loopcount>\n", argv[0]);
      exit(1);
  }

  /* Parse argument */
  n          = min(atol(argv[1]), MAX_THREAD_NUM);
  loop_count = atol(argv[2]); /* Don't bother with format checking */

  memset(counters, 0, sizeof(counters));

  /* Start the threads */
  for (int64_t i = 0; i < n; ++i) {
    pthread_create(&threads[i], NULL, (void* (*)(void*))worker, &(counters[i].val));
  }

  for (int64_t i = 0; i < n; ++i) {
    pthread_join(threads[i], NULL);
  }

  int64_t counter = 0;
  for (int64_t i = 0; i < n; ++i) {
    printf("%ld ", counters[i].val);
    counter += counters[i].val;
  }
  printf("\n");
  printf("sum: %ld\n", counter);

  if ((loop_count-1)*loop_count/2*n != counter) {
    printf("corrupted queue, counter: %ld != total: %ld\n", counter, (loop_count-1)*loop_count/2*n);
  } else {
    printf("works fine!\n");
  }


  return 0;
}
