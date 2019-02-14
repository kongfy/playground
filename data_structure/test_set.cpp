#include "stdio.h"
#include "pthread.h"
#include <algorithm>

#include "linked_set.h"
#include "lock/clhlock.h"

using namespace std;

static int64_t n          = 0;
static int64_t loop_count = 0;

static Set<int64_t> set;

static CLHLock lock;

struct counter {
  int64_t add_succ_cnt;
  int64_t add_fail_cnt;
  int64_t remove_cnt;

  counter() : add_succ_cnt(0), add_fail_cnt(0), remove_cnt(0) {}

  void print()
  {
    printf("%20ld\t%20ld\t%20ld\n", add_succ_cnt, add_fail_cnt, remove_cnt);
  }

  counter &operator += (const counter &o)
  {
    add_succ_cnt += o.add_succ_cnt;
    add_fail_cnt += o.add_fail_cnt;
    remove_cnt += o.remove_cnt;
    return *this;
  }

} CACHE_ALIGNED counters[MAX_THREAD_NUM];

void worker(const int &pid)
{
  counter &cnt = counters[pid];

  for (int64_t i = 0; i < loop_count; ++i) {
    int ret = 0;
    int64_t add_key = i * (pid + 1);
    int64_t remove_key = i * (n - pid);

    // ListLockGuard guard(lock);

    // add
    if ((ret = set.add(add_key)) >= 0) {
      cnt.add_succ_cnt += add_key;
    } else if (SET_ENTRY_DUPLICATE == ret) {
      cnt.add_fail_cnt += add_key;
    } else {
      printf("ERROR: unknown error code %d", ret);
    }

    // remove
    if ((ret = set.remove(remove_key)) >= 0) {
      cnt.remove_cnt += remove_key;
    }
  }
}

void validate()
{
  struct counter total_cnt;

  printf("            add succ\t            add fail\t              remove\n");
  for (int64_t i = 0; i < n; ++i) {
    counters[i].print();
    total_cnt += counters[i];
  }
  printf("==============\n");
  printf("total count\n");
  total_cnt.print();

  int64_t set_cnt = 0;
  Set<int64_t>::Iterator it;
  int64_t last = 0;
  for (it = set.begin(); it != set.end(); ++it) {
    int64_t tmp = *it;
    if (tmp <= last) {
      printf("corrupted set: ... %ld %ld ...\n", last, tmp);
      return;
    } else {
      last = tmp;
    }
    set_cnt += *it;
  }

  if (total_cnt.add_succ_cnt != total_cnt.remove_cnt + set_cnt) {
    printf("corrupted set, set_cnt: %ld total: %ld\n", set_cnt, set_cnt + total_cnt.remove_cnt);
  } else {
    printf("works fine!\n");
  }
}

int main(int argc, char *argv[])
{
  pthread_t threads[MAX_THREAD_NUM];
  int pids[MAX_THREAD_NUM];

  /* Check arguments to program*/
  if(argc != 3) {
      fprintf(stderr, "USAGE: %s <threads> <loopcount>\n", argv[0]);
      exit(1);
  }

  /* Parse argument */
  n          = min(atol(argv[1]), MAX_THREAD_NUM);
  loop_count = atol(argv[2]); /* Don't bother with format checking */

  for (int64_t i = 0; i < n; ++i) {
    pids[i] = i;
  }

  /* Start the threads */
  for (int64_t i = 0; i < n; ++i) {
    pthread_create(&threads[i], NULL, (void* (*)(void*))worker, &pids[i]);
  }

  for (int64_t i = 0; i < n; ++i) {
    pthread_join(threads[i], NULL);
  }

  validate();

  return 0;
}
