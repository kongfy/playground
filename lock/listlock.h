#ifndef _LISTLOCK_H_
#define _LISTLOCK_H_

class ListLock
{
public:
  class qnode
  {
  public:
    qnode *p_qnode;
    bool locked;

    void *operator new(size_t request)
    {
      static const size_t needed = CACHE_ALIGN_SIZE + sizeof(void *) + sizeof(qnode);

      void *alloc = ::operator new(needed);
      void *ptr = (void *)((((unsigned long)alloc + CACHE_ALIGN_SIZE) & ~(CACHE_ALIGN_SIZE - 1)) + sizeof(void *));

      ((void **)ptr)[-1] = alloc; // save for delete calls to use
      return ptr;
    }

    void operator delete(void *ptr)
    {
      if (ptr) { // 0 is valid, but a noop, so prevent passing negative memory
        void * alloc = ((void **)ptr)[-1];
        ::operator delete (alloc);
      }
    }
  };

  ListLock() {};
  virtual ~ListLock() {};

  ListLock(const ListLock&) = delete;
  ListLock &operator=(const ListLock&) = delete;

  virtual int lock(qnode *node) = 0;
  virtual int unlock(qnode *&node) = 0;
};

class ListLockGuard
{
public:
  ListLockGuard(ListLock &lock) : lock_(lock)
  {
    node_ = new ListLock::qnode();
    lock_.lock(node_);
  }

  virtual ~ListLockGuard()
  {
    lock_.unlock(node_);
    delete node_;
  }

  ListLockGuard(const ListLockGuard&) = delete;
  ListLockGuard &operator=(const ListLockGuard&) = delete;
private:
  ListLock &lock_;
  ListLock::qnode *node_;
};

#endif /* _LISTLOCK_H_ */
