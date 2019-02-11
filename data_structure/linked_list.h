#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

#include "common.h"

static const int LIST_ENTRY_DUPLICATE = -1;
static const int LIST_ENTRY_NOT_EXSIT = -2;

template <typename T>
class LinkedList
{
public:
  LinkedList() : head_(NULL), tail_(NULL)
  {
    head_ = tail_ = new qnode();
    head_->next = NULL;
  }

  virtual ~LinkedList()
  {
    // TODO: recycle memory safely
  }

  int insert(const T key);
  int remove(const T key);
  bool find(const T key);

private:
  class qnode
  {
  public:
    qnode() {}
    qnode(T d) : data(d), next(NULL) {}
    T data;
    qnode *next;
  };

  void search(const T key, qnode *&prev, qnode*&next);

  qnode *head_ CACHE_ALIGNED;
  qnode *tail_ CACHE_ALIGNED;

public:
  class Iterator {
    friend LinkedList<T>;
  public:
    Iterator &operator ++() { if (curr_ != NULL) curr_ = curr_->next; return *this; }
    bool operator != (const Iterator &o) const { return curr_ != o.curr_; }
    T &operator * () { if (curr_ != NULL) return curr_->data; }
  private:
    qnode *curr_;
  };

  Iterator begin();
  Iterator end();
};

template <typename T>
int LinkedList<T>::insert(const T key)
{
  qnode *prev = NULL;
  qnode *next = NULL;

  search(key, prev, next);

  if (next && next->data == key) {
    return LIST_ENTRY_DUPLICATE;
  }

  qnode *tmp = new qnode(key);
  tmp->next = next;
  prev->next = tmp;

  return 0;
}

template <typename T>
int LinkedList<T>::remove(const T key)
{
  qnode *prev = NULL;
  qnode *next = NULL;

  search(key, prev, next);

  if (next && next->data == key) {
    prev->next = next->next;
    // TODO: free qnode
  } else {
    return LIST_ENTRY_NOT_EXSIT;
  }

  return 0;
}

template <typename T>
bool LinkedList<T>::find(const T key)
{
  qnode *prev = NULL;
  qnode *next = NULL;

  search(key, prev, next);

  if (next && next->data == key) {
    return true;
  } else {
    return false;
  }
}

template <typename T>
void LinkedList<T>::search(const T key, qnode *&prev, qnode*&next)
{
  prev = head_;
  next = prev->next;

  while (next && next->data < key) {
    prev = next;
    next = prev->next;
  }
}

template <typename T>
typename LinkedList<T>::Iterator LinkedList<T>::begin()
{
  Iterator iter;
  iter.curr_ = head_->next;
  return iter;
}

template <typename T>
typename LinkedList<T>::Iterator LinkedList<T>::end()
{
  Iterator iter;
  iter.curr_ = NULL;
  return iter;
}

#endif /* _LINKED_LIST_H_ */
