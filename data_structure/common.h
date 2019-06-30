#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <stdint.h>

#define CACHE_ALIGN_SIZE 64
#define CACHE_ALIGNED __attribute__((aligned(CACHE_ALIGN_SIZE)))

static const int64_t MAX_THREAD_NUM = 128;

int64_t __next_tid __attribute__((weak));
inline int64_t get_itid()
{
  static __thread int64_t tid = -1;
  return tid < 0 ? (tid = __sync_fetch_and_add(&__next_tid, 1)) : tid;
}

#endif /* _COMMON_H_ */
