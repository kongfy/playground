#ifndef _MUTEX_H_
#define _MUTEX_H_

#include <pthread.h>

#include "lock.h"

class MutexLock : public BaseLock
{
public:
  MutexLock()
  {
    pthread_mutex_init(&lock_, NULL);
  }

  virtual ~MutexLock() override
  {
    pthread_mutex_destroy(&lock_);
  }

  virtual int lock() override
  {
    return pthread_mutex_lock(&lock_);
  }

  virtual int unlock() override
  {
    return pthread_mutex_unlock(&lock_);
  }
private:
  pthread_mutex_t lock_;
};

#endif /* _MUTEX_H_ */
