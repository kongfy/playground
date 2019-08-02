#ifndef _RCU_COUNTER_H_
#define _RCU_COUNTER_H_

#include "common.h"
#include "rcu.h"

class RCUCounter : public RCUMgr
{
public:
  RCUCounter() : g_cnt_(1) { memset(&storage_, 0, sizeof(storage_)); }
  ~RCUCounter() {}

  void rcu_read_lock() override;
  void rcu_read_unlock() override;
  void synchronize_rcu() override;
  void call_rcu(const void* const p, rcu_cb cb) override;
private:
  struct threadlocal
  {
    uint64_t cnt;
  } CACHE_ALIGNED;

  uint64_t g_cnt_ CACHE_ALIGNED;
  threadlocal storage_[MAX_THREAD_NUM];
};

void RCUCounter::rcu_read_lock()
{
  storage_[get_itid()].cnt = g_cnt_;

  // __sync_synchronize();
  __atomic_thread_fence(__ATOMIC_ACQUIRE);
}

void RCUCounter::rcu_read_unlock()
{
  // __sync_synchronize();
  __atomic_thread_fence(__ATOMIC_RELEASE);

  storage_[get_itid()].cnt = 0;
}

void RCUCounter::synchronize_rcu()
{
  uint64_t cnt = __sync_add_and_fetch(&g_cnt_, 2);

  for (int64_t i = 0; i < MAX_THREAD_NUM; ++i) {
    while (storage_[i].cnt != 0 && storage_[i].cnt < cnt) {
      asm volatile("pause\n" ::: "memory");
    }
  }
}

// fake...
void RCUCounter::call_rcu(const void* const p, rcu_cb cb)
{
  synchronize_rcu();
  cb(p);
}

#endif /* _RCU_COUNTER_H_ */
