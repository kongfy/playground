#ifndef _LOCK_H_
#define _LOCK_H_

#include <stdio.h>

class BaseLock
{
public:
  BaseLock() {};
  virtual ~BaseLock() {};

  BaseLock(const BaseLock&) = delete;
  BaseLock &operator=(const BaseLock&) = delete;

  virtual int lock() = 0;
  virtual int unlock() = 0;
};


class BaseLockGuard
{
public:
  BaseLockGuard(BaseLock &lock) : lock_(lock)
  {
    if (lock_.lock()) {
      printf("lock failed.\n");
    }
  }

  virtual ~BaseLockGuard()
  {
    if (lock_.unlock()) {
      printf("unlock failed.\n");
    }
  }

  BaseLockGuard(const BaseLockGuard&) = delete;
  BaseLockGuard &operator=(const BaseLockGuard&) = delete;
private:
  BaseLock &lock_;
};

#endif /* _LOCK_H_ */
