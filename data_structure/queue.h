#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "common.h"
#include "hazard_pointer.h"

template <typename T>
class Queue
{
public:
  Queue() : head_(NULL), tail_(NULL), hazard_mgr_(2)
  {
    head_ = tail_ = new qnode();
    head_->next = NULL;
  }

  virtual ~Queue()
  {
    T tmp;
    while (dequeue(tmp)) {
    }

    delete head_;
  }

  void enqueue(const T &data);
  bool dequeue(T &data);
private:
  class qnode
  {
  public:
    T data;
    qnode *next;
  };

  qnode *head_ CACHE_ALIGNED;
  qnode *tail_ CACHE_ALIGNED;
  HazardManager hazard_mgr_;
};

template <typename T>
void Queue<T>::enqueue(const T &data)
{
  qnode *node = new qnode();
  node->data = data;
  node->next = NULL;
  // qnode *t = NULL;
  HazardPointer<qnode> t(hazard_mgr_);
  qnode *next = NULL;

  while (true) {
    if (!t.acquire(&tail_)) {
      continue;
    }
    next = t->next;
    if (next) {
      __sync_bool_compare_and_swap(&tail_, t, next);
      continue;
    }
    if (__sync_bool_compare_and_swap(&t->next, NULL, node)) {
      break;
    }
  }
  __sync_bool_compare_and_swap(&tail_, t, node);
}

template <typename T>
bool Queue<T>::dequeue(T &data)
{
  qnode *t = NULL;
  // qnode *h = NULL;
  HazardPointer<qnode> h(hazard_mgr_);
  // qnode *next = NULL;
  HazardPointer<qnode> next(hazard_mgr_);

  while (true) {
    if (!h.acquire(&head_)) {
      continue;
    }
    t = tail_;
    next.acquire(&h->next);
    asm volatile("" ::: "memory");
    if (head_ != h) {
      continue;
    }
    if (!next) {
      return false;
    }
    if (h == t) {
      __sync_bool_compare_and_swap(&tail_, t, next);
      continue;
    }
    data = next->data;
    if (__sync_bool_compare_and_swap(&head_, h, next)) {
      break;
    }
  }

  /* h->next = (qnode *)1; // bad address, It's a trap! */
  /* delete h; */
  hazard_mgr_.retireNode(h);
  return true;
}

#endif /* _QUEUE_H_ */
