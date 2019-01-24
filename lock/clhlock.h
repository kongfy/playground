#ifndef _CLHLOCK_H_
#define _CLHLOCK_H_

#include "listlock.h"

class CLHLock : public ListLock
{
public:
  CLHLock()
  {
    lock_ = new qnode();
    lock_->locked = false;
  }

  virtual ~CLHLock()
  {
    delete lock_;
  }

  virtual int lock(qnode *node) override
  {
    node->locked = true;

    qnode *prev = node->p_qnode = __sync_lock_test_and_set(&lock_, node);

    while (prev->locked) {
      asm volatile("pause\n" ::: "memory");
    }
    return 0;
  }

  virtual int unlock(qnode *&node) override
  {
    qnode *t = node->p_qnode;
    node->locked = false;
    node = t;
    return 0;
  }
private:
  qnode *lock_;
};


#endif /* _CLHLOCK_H_ */
