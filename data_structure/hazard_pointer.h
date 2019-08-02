#ifndef _HAZARD_POINTER_H_
#define _HAZARD_POINTER_H_

#include "common.h"

static const int64_t MAX_K = 4;

class HazardManager
{
  template <typename T> friend class HazardPointer;
public:
  HazardManager(int k);
  virtual ~HazardManager();

  typedef void (*retire_cb)(const void* const node);
  void retireNode(const void* const node, retire_cb cb);

  HazardManager(const HazardManager&) = delete;
  HazardManager &operator=(const HazardManager&) = delete;
private:
  struct rnode
  {
    const void *node;
    retire_cb cb;
    rnode *next;

    rnode() : node(NULL), cb(NULL), next(NULL) {}
  };
  struct threadlocal
  {
    const void* hp[MAX_K];
    int64_t pcount;
    rnode *rlist CACHE_ALIGNED;
    int64_t rcount;

    threadlocal() : pcount(0), rlist(), rcount(0)
    {
      rlist = new rnode();
    }

    ~threadlocal()
    {
      delete rlist;
    }
  } CACHE_ALIGNED;

  void retireNode(const void* const node, const int64_t &tid, retire_cb cb);
  void scan(threadlocal &data);
  bool acquire(const void* const node, const int64_t &tid);
  void release(const void* const node, const int64_t tid);

  int k_;
  int hash_size_;
  unsigned long hash_mask_;
  threadlocal storage_[MAX_THREAD_NUM];
};

template <typename T>
class HazardPointer
{
public:
  HazardPointer(HazardManager &mgr) : tid_(), node_(NULL), mgr_(mgr)
  {
    tid_ = get_itid();
  }

  ~HazardPointer()
  {
    release();
  }

  HazardPointer(const HazardPointer&) = delete;
  HazardPointer &operator=(const HazardPointer) = delete;

  operator T*() const { return const_cast<T*>(node_); }
  T* operator->() const { return const_cast<T*>(node_); }
  T& operator*() const { return *node_; }

  bool acquire(const T* const *node);
  void release();
private:
  int64_t tid_;
  const T *node_;
  HazardManager &mgr_;
};

template <typename T>
bool HazardPointer<T>::acquire(const T* const *node)
{
  release();

  node_ = *node;
  if (!mgr_.acquire(node_, tid_)) {
    return false;
  }
  // continuosly holding, need sequential consistency
  __sync_synchronize();
  if (*node != node_) {
    release();
    return false;
  }

  return true;
}

template <typename T>
void HazardPointer<T>::release()
{
  if (node_) {
    mgr_.release(node_, tid_);
    node_ = NULL;
  }
}

#endif /* _HAZARD_POINTER_H_ */
