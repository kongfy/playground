#ifndef _RCU_COUNTER_H_
#define _RCU_COUNTER_H_

#include "common.h"
#include "rcu.h"

static const uint64_t NEST_SHIFT = 6L;
static const uint64_t NEST_BIT   = 1L << NEST_SHIFT;
static const uint64_t NEST_MASK  = NEST_BIT - 1L;

struct threadlocal
{
  uint64_t cnt;
} CACHE_ALIGNED;

volatile static uint64_t g_cnt_ CACHE_ALIGNED = 1;
static threadlocal storage_[MAX_THREAD_NUM];

void rcu_read_lock()
{
  if (storage_[get_itid()].cnt & NEST_MASK) {
    storage_[get_itid()].cnt += 1;
  } else {
    storage_[get_itid()].cnt = g_cnt_;
  }

  // __sync_synchronize();
  __atomic_thread_fence(__ATOMIC_ACQUIRE);
}

void rcu_read_unlock()
{
  // __sync_synchronize();
  __atomic_thread_fence(__ATOMIC_RELEASE);

  storage_[get_itid()].cnt -= 1;
}

void synchronize_rcu()
{
  uint64_t cnt = __sync_add_and_fetch(&g_cnt_, NEST_BIT);

  for (int64_t i = 0; i < MAX_THREAD_NUM; ++i) {
    while ((storage_[i].cnt & NEST_MASK) != 0 && storage_[i].cnt < cnt) {
      asm volatile("pause\n" ::: "memory");
    }
  }
}

// fake...
void call_rcu(const void* const p, rcu_cb cb)
{
  synchronize_rcu();
  cb(p);
}

#endif /* _RCU_COUNTER_H_ */
