#ifndef _STACK_H_
#define _STACK_H_

#include "common.h"

#include "hazard_pointer.h"

template <typename T>
class Stack
{
public:
  Stack() : top_(NULL), hazard_mgr_(1)
  {
  }

  ~Stack()
  {
    T tmp;
    while (pop(tmp)) {}
  }

  void push(const T &data);
  bool pop(T &data);
private:
  class qnode
  {
  public:
    T data;
    qnode *next;
  };

  static void free_node(const void *node)
  {
    delete (const qnode*)node;
  }

  qnode *top_ CACHE_ALIGNED;
  HazardManager hazard_mgr_;
};

template <typename T>
void Stack<T>::push(const T &data)
{
  qnode *t = new qnode();
  t->data = data;

  while (true) {
    t->next = top_;
    if (__sync_bool_compare_and_swap(&top_, t->next, t)) {
      return;
    }
    asm volatile("pause\n");
  }
}

template<typename T>
bool Stack<T>::pop(T &data)
{
  HazardPointer<qnode> t(hazard_mgr_);

  while (true) {
    if (!t.acquire(&top_)) {
      continue;
    }

    if (!t) {
      return false;
    }

    if (__sync_bool_compare_and_swap(&top_, t, t->next)) {
      data = t->data;
      hazard_mgr_.retireNode(t, &free_node);
      return true;
    }

    asm volatile("pause\n");
  }
}

#endif /* _STACK_H_ */
