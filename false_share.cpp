/*
 * Demo program for showing the drawback of "false sharing"
 *
 * Use it with perf!
 *
 * Compile: g++ -O2 -o false_share false_share.cpp -lpthread
 * Usage: perf stat -e cache-misses ./false_share <loopcount> <is_aligned>
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

#define CACHE_ALIGN_SIZE 64
#define CACHE_ALIGNED __attribute__((aligned(CACHE_ALIGN_SIZE)))

int gLoopCount;

inline int64_t current_time()
{
  struct timeval t;
  if (gettimeofday(&t, NULL) < 0) {
  }
  return (static_cast<int64_t>(t.tv_sec) * static_cast<int64_t>(1000000) + static_cast<int64_t>(t.tv_usec));
}

struct value {
  int64_t val;
};
value data[2] CACHE_ALIGNED;

struct aligned_value {
  int64_t val;
} CACHE_ALIGNED;
aligned_value aligned_data[2] CACHE_ALIGNED;

void* worker1(int64_t *val)
{
  printf("worker1 start...\n");

  volatile int64_t &v = *val;
  for (int i = 0; i < gLoopCount; ++i) {
    v += 1;
  }

  printf("worker1 exit...\n");
}

// duplicate worker function for perf report
void* worker2(int64_t *val)
{
  printf("worker2 start...\n");

  volatile int64_t &v = *val;
  for (int i = 0; i < gLoopCount; ++i) {
    v += 1;
  }

  printf("worker2 exit...\n");
}

int main(int argc, char *argv[])
{
  pthread_t race_thread_1;
  pthread_t race_thread_2;

  bool is_aligned;

  /* Check arguments to program*/
  if(argc != 3) {
    fprintf(stderr, "USAGE: %s <loopcount> <is_aligned>\n", argv[0]);
    exit(1);
  }

  /* Parse argument */
  gLoopCount = atoi(argv[1]); /* Don't bother with format checking */
  is_aligned = atoi(argv[2]); /* Don't bother with format checking */

  printf("size of unaligned data : %d\n", sizeof(data));
  printf("size of aligned data   : %d\n", sizeof(aligned_data));

  void *val_0, *val_1;
  if (is_aligned) {
    val_0 = (void *)&aligned_data[0].val;
    val_1 = (void *)&aligned_data[1].val;
  } else {
    val_0 = (void *)&data[0].val;
    val_1 = (void *)&data[1].val;
  }

  int64_t start_time = current_time();

  /* Start the threads */
  pthread_create(&race_thread_1, NULL, (void* (*)(void*))worker1, val_0);
  pthread_create(&race_thread_2, NULL, (void* (*)(void*))worker2, val_1);

  /* Wait for the threads to end */
  pthread_join(race_thread_1, NULL);
  pthread_join(race_thread_2, NULL);

  int64_t end_time = current_time();

  printf("time : %d us\n", end_time - start_time);

  return 0;
}
