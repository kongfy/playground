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

  static qnode *mark(qnode *p)
  {
    return (qnode*) ((uint64_t) p | 1L);
  }

  static qnode *unmark(qnode *p)
  {
    return (qnode*) ((uint64_t) p & ~1L);
  }

  static bool is_marked(qnode *p)
  {
    return (uint64_t) p & 1L;
  }

  void search(const T key, qnode *&prev, qnode*&curr);

  qnode *head_ CACHE_ALIGNED;
  qnode *tail_ CACHE_ALIGNED;

public:
  class Iterator {
    friend LinkedList<T>;
  public:
    Iterator &operator ++()
    {
      if (curr_ != NULL) {
        do {
          curr_ = unmark(curr_->next);
        } while (curr_ && is_marked(curr_->next));
      }
      return *this;
    }
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
  qnode *curr = NULL;
  qnode *tmp = new qnode(key);

  while (true) {
    search(key, prev, curr);

    if (curr && curr->data == key) {
      free(tmp);
      return LIST_ENTRY_DUPLICATE;
    }

    tmp->next = curr;
    if (__sync_bool_compare_and_swap(&prev->next, curr, tmp)) {
      break;
    }
  }

  return 0;
}

template <typename T>
int LinkedList<T>::remove(const T key)
{
  qnode *prev = NULL;
  qnode *curr = NULL;
  qnode *next = NULL;

  while (true) {
    search(key, prev, curr);

    if (curr && curr->data == key) {
      next = curr->next;
      if (!is_marked(next)) {
        if (__sync_bool_compare_and_swap(&curr->next, next, mark(next))) {
          break;
        }
      }
    } else {
      return LIST_ENTRY_NOT_EXSIT;
    }
  }

  if (__sync_bool_compare_and_swap(&prev->next, curr, next)) {
    // TODO: free qnode
  }

  return 0;
}

template <typename T>
bool LinkedList<T>::find(const T key)
{
  qnode *prev = NULL;
  qnode *curr = NULL;

  search(key, prev, curr);

  if (curr && curr->data == key) {
    return true;
  } else {
    return false;
  }
}

template <typename T>
void LinkedList<T>::search(const T key, qnode *&prev, qnode*&curr)
{
  qnode *t = NULL;
  bool retry = false;

  do {
    retry = false;

    prev = head_;
    curr = unmark(prev->next);

    while (curr) {
      t = curr->next;

      while (curr && is_marked(t)) {
        if (__sync_bool_compare_and_swap(&prev->next, curr, unmark(t))) {
          // TODO: free qnode
          curr = unmark(t);
          if (curr) t = curr->next;
        } else {
          retry = true;
          break;
        }
      }

      if (retry) break;

      if (!curr || curr->data >= key) {
        return;
      } else {
        prev = curr;
        curr = unmark(prev->next);
      }
    }
  } while (retry);

  // while (true) {
  //   qnode *t = head_;
  //   qnode *t_next = NULL;
  //   qnode *prev_next = NULL;

  //   do {
  //     t_next = t->next;
  //     if (!is_marked(t_next)) {
  //       prev = t;
  //       prev_next = t_next;
  //     }
  //     t = unmark(t_next);
  //   } while (t && (is_marked(t->next) || t->data < key));

  //   curr = t;

  //   if (prev_next == curr) {
  //     if (curr && is_marked(curr->next)) {
  //       continue;
  //     } else {
  //       return;
  //     }
  //   }

  //   if (__sync_bool_compare_and_swap(&prev->next, prev_next, curr)) {
  //     if (curr && is_marked(curr->next)) {
  //       continue;
  //     } else {
  //       return;
  //     }
  //   }
  // }

  return;
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
