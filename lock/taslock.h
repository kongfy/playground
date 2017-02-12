#ifndef _TASLOCK_H_
#define _TASLOCK_H_

#include "lock.h"

class TASLock : public BaseLock
{
public:
  TASLock() : lock_(0L)
  {
  }

  virtual ~TASLock() override
  {
  }

  virtual int lock() override
  {
    while (__sync_lock_test_and_set(&lock_, 1L)) {
      asm volatile("pause\n");
    }
    return 0;
  }

  virtual int unlock() override
  {
    lock_ = 0L;
    return 0;
  }

private:
  int64_t lock_;
};

#endif /* _TASLOCK_H_ */
