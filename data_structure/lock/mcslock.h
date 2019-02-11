#ifndef _MCSLOCK_H_
#define _MCSLOCK_H_

class MCSLock : public ListLock
{
public:
  MCSLock() : lock_(NULL)
  {
  }

  virtual ~MCSLock()
  {
  }

  virtual int lock(qnode *node) override
  {
    node->p_qnode = NULL;

    qnode *prev = __sync_lock_test_and_set(&lock_, node);
    // ATTENTION: time window here.
    if (prev) {
      node->locked = true;
      prev->p_qnode = node;

      while (node->locked) {
        asm volatile("pause\n" ::: "memory");
      }
    }
    return 0;
  }

  virtual int unlock(qnode *&node) override
  {
    if (!node->p_qnode) {
      if (__sync_bool_compare_and_swap(&lock_, node, NULL)) {
        return 0;
      }
      // take care of time window
      while (!node->p_qnode) {
        asm volatile("pause\n" ::: "memory");
      }
    }
    node->p_qnode->locked = false;
    return 0;
  }
private:
  qnode *lock_;
};

#endif /* _MCSLOCK_H_ */
