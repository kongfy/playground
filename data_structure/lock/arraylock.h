#ifndef _ARRAYLOCK_H_
#define _ARRAYLOCK_H_

#include <utmpx.h>
#include <string.h>

#include "common.h"

class ArrayLock
{
public:
  ArrayLock() : pack_(0L)
  {
    memset(slots_, 0L, sizeof(slots_));
    pack_ = (unsigned long)&slots_[0] | 1Lu;
  }

  virtual ~ArrayLock() {}

  ArrayLock(const ArrayLock&) = delete;
  ArrayLock &operator=(const ArrayLock&) = delete;

  virtual int lock(const int64_t tid)
  {
    if (tid < 0) return -1;

    unsigned long pack = __sync_lock_test_and_set(&pack_, (unsigned long)&slots_[tid] | (unsigned long)slots_[tid].v);
    bool *tail = &(((aligned_bool *)(pack & ~1Lu))->v);
    bool locked = pack & 1Lu;

    while (locked == *tail) {
      asm volatile("pause\n" ::: "memory");
    }
    return 0;
  }

  virtual int unlock(const int64_t tid)
  {
    if (tid < 0) return -1;

    slots_[tid].v = !slots_[tid].v;
    return 0;
  }

private:
  struct aligned_bool{
    bool v;
  } CACHE_ALIGNED slots_[MAX_THREAD_NUM];
  unsigned long pack_; // last bit is "this_means_locked"
};

class ArrayLockGuard
{
public:
  ArrayLockGuard(ArrayLock &lock, const int64_t tid) : lock_(lock), tid_(tid)
  {
    if (lock_.lock(tid_)) {
      printf("lock failed.\n");
    }
  }

  virtual ~ArrayLockGuard()
  {
    if (lock_.unlock(tid_)) {
      printf("unlock failed.\n");
    }
  }

  ArrayLockGuard(const ArrayLockGuard&) = delete;
  ArrayLockGuard &operator=(const ArrayLockGuard&) = delete;
private:
  ArrayLock &lock_;
  const int64_t tid_;
};

#endif /* _ARRAYLOCK_H_ */
