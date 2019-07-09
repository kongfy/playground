#ifndef _BAKERYLOCK_H_
#define _BAKERYLOCK_H_

#include "common.h"

class BakeryLock
{
public:
  BakeryLock() {
    memset(entering_, 0, sizeof(entering_));
    memset(number_, 0, sizeof(number_));
  };
  ~BakeryLock() {};

  BakeryLock(const BakeryLock&) = delete;
  BakeryLock &operator=(const BakeryLock &) = delete;

  int lock(const int64_t tid)
  {
    entering_[tid].v = true;

    __sync_synchronize();

    int64_t number = 0;
    for (int64_t i = 0; i < MAX_THREAD_NUM; ++i) {
      if (number_[i].v > number) {
        number = number_[i].v;
      }
    }

    number += 1;
    number_[tid].v = number;
    asm volatile("" ::: "memory");
    entering_[tid].v = false;

    __sync_synchronize();

    int64_t t_number = 0;
    for (int64_t i = 0; i < MAX_THREAD_NUM; ++i) {
      while (*(volatile bool *)&entering_[i].v) {
        asm ("pause\n");
      }

      asm volatile("" ::: "memory");
      while (true) {
        t_number = *(volatile int64_t *)&number_[i].v;
        if (t_number != 0 && (t_number < number || (t_number == number && i < tid))) {
          asm ("pause\n");
        } else {
          break;
        }
      }
    }

    return 0;
  }

  int unlock(const int64_t tid)
  {
    number_[tid].v = 0;
    return 0;
  }

private:
  struct aligned_bool {
    bool v;
  } CACHE_ALIGNED entering_[MAX_THREAD_NUM];
  struct aligned_int {
    int64_t v;
  } CACHE_ALIGNED number_[MAX_THREAD_NUM];
};

class BakeryLockGuard
{
public:
  BakeryLockGuard(BakeryLock &lock, const int64_t tid) : lock_(lock), tid_(tid)
  {
    if (lock_.lock(tid_)) {
      printf("lock failed.\n");
    }
  }

  ~BakeryLockGuard()
  {
    if (lock_.unlock(tid_)) {
      printf("unlock failed.\n");
    }
  }

  BakeryLockGuard(const BakeryLockGuard&) = delete;
  BakeryLockGuard &operator=(const BakeryLockGuard&) = delete;

private:
  BakeryLock &lock_;
  const int64_t tid_;
};


#endif /* _BAKERYLOCK_H_ */
