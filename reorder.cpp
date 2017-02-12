/*
 * Demo program for catching cpu reorder behaviors
 *
 * Compile: g++ -O2 -o reorder reorder.cpp -lpthread
 * Usage: ./reorder <loopcount>
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

int gLoopCount;
int A, B, X, Y;

inline int64_t current_time()
{
  struct timeval t;
  if (gettimeofday(&t, NULL) < 0) {
  }
  return (static_cast<int64_t>(t.tv_sec) * static_cast<int64_t>(1000000) + static_cast<int64_t>(t.tv_usec));
}

void* worker1(void *arg)
{
  X = 1;
  asm volatile("" ::: "memory");
  A = Y;
}

void* worker2(void *arg)
{
  Y = 1;
  asm volatile("" ::: "memory");
  B = X;
}


int main(int argc, char *argv[])
{
  pthread_t race_thread_1;
  pthread_t race_thread_2;

  int64_t count = 0;

  /* Check arguments to program*/
  if(argc != 2) {
    fprintf(stderr, "USAGE: %s <loopcount>\n", argv[0]);
    exit(1);
  }

  /* Parse argument */
  gLoopCount = atoi(argv[1]); /* Don't bother with format checking */

  for (int i = 0; i < gLoopCount; ++i) {
    X = 0;
    Y = 0;

    /* Start the threads */
    pthread_create(&race_thread_1, NULL, (void* (*)(void*))worker1, NULL);
    pthread_create(&race_thread_2, NULL, (void* (*)(void*))worker2, NULL);

    /* Wait for the threads to end */
    pthread_join(race_thread_1, NULL);
    pthread_join(race_thread_2, NULL);

    if (A == 0 && B == 0) {
      printf("reorder caught!\n");
      count++;
    }

  }

  printf("%d reorder cought in %d iterations.\n", count, gLoopCount);

  return 0;
}
