#ifndef _SKIPLIST_H_
#define _SKIPLIST_H_

#include <string.h>

#include "set.h"
#include "linked_list.h"
#include "rcu.h"

template <typename T>
class OrderedSet
{
private:
  static const int MAX_LEVEL = 15;

  class list_node
  {
  public:
    // sentinel
    list_node() : key_(), top_level_(MAX_LEVEL), next_()
    {
      memset(next_, 0, sizeof(next_));
    }

    // normal node
    list_node(const T &key, const unsigned char &height) : key_(key), top_level_(height), next_()
    {
      memset(next_, 0, sizeof(next_));
    }

    T key_;
    unsigned char top_level_;
    list_node *next_[MAX_LEVEL + 1]; // TODO: reduce memory cost
  };

public:
  OrderedSet() : head_(), tail_()
  {
    for (int64_t i = 0; i <= MAX_LEVEL; ++i) {
      head_.next_[i] = &tail_;
    }
  }

  int add(const T &key);
  int remove(const T &key);
  bool contains(const T &key);

  class Iterator {
    friend OrderedSet<T>;
  public:
    Iterator() : curr_(NULL) {}
    Iterator &operator ++()
    {
      list_node *next = NULL;
      if (curr_ && (next = curr_->next_[0])) {
        curr_ = unmark(next);
        next = curr_->next_[0];
        while (is_marked(next)) {
          curr_ = unmark(next);
          next = curr_->next_[0];
        }
      }
      return *this;
    }
    bool operator != (const Iterator &o) const { return curr_ != o.curr_; }
    T &operator * () { if (curr_ && curr_->next_[0]) { return curr_->key_; } else { abort(); } }
  private:
    list_node *curr_;
    RCUReadGuard guard_;
  };

  Iterator begin()
  {
    Iterator iter;
    iter.curr_ = head_.next_[0];
    return iter;
  }

  Iterator end()
  {
    Iterator iter;
    iter.curr_ = &tail_;
    return iter;
  }

private:
  static unsigned char random_height()
  {
    unsigned char height = 0;
    int cnt = 0;

    while (cnt < MAX_LEVEL) {
      int t = rand(); // [0, 32767]

      for (int64_t i = 0; i < 15; ++i) {
        if (t & 1L) {
          height++;
          t >>= 1L;
        } else {
          return height;
        }
        ++cnt;
      }
    }

    return height;
  }

  static list_node *mark(list_node * const p)
  {
    return (list_node*) ((uint64_t) p | 1L);
  }

  static list_node *unmark(list_node * const p)
  {
    return (list_node*) ((uint64_t) p & ~1L);
  }

  static bool is_marked(list_node * const p)
  {
    return (uint64_t) p & 1L;
  }

  bool find(const T &key, list_node *prevs[], list_node *currs[]);

  list_node head_;
  list_node tail_;
};


template <typename T>
int OrderedSet<T>::add(const T &key)
{
  list_node *prevs[MAX_LEVEL + 1];
  list_node *currs[MAX_LEVEL + 1];
  const unsigned char top_level = random_height();
  list_node *new_node = new list_node(key, top_level);

  RCUReadGuard guard();

  // add bottom level
  while (true) {
    if (find(key, prevs, currs)) {
      delete new_node;
      return SET_ENTRY_DUPLICATE;
    } else {
      for (int64_t i = 0; i <= top_level; ++i) {
        new_node->next_[i] = currs[i];
      }

      list_node *prev = prevs[0];
      list_node *curr = currs[0];

      if (__sync_bool_compare_and_swap(&prev->next_[0], curr, new_node)) {
        break;
      }
    }
  }

  // add upper levels
  for (int64_t i = 1; i <= top_level; ++i) {
    while (true) {
      list_node *prev = prevs[i];
      list_node *curr = currs[i];

      if (!__sync_bool_compare_and_swap(&prev->next_[i], curr, new_node)) {
        find(key, prevs, currs);

        // change new_node's next_[i..top_level]
        for (int64_t j = i; j <= top_level; ++j) {
          list_node *next = new_node->next_[j];
          if (is_marked(next)
              || !__sync_bool_compare_and_swap(&new_node->next_[j], next, currs[j])) {
            // new_node has been removed
            return 0;
          }
        }
      } else {
        break;
      }
    }
  }

  return 0;
}

template <typename T>
int OrderedSet<T>::remove(const T &key)
{
  list_node *prevs[MAX_LEVEL + 1];
  list_node *currs[MAX_LEVEL + 1];

  RCUReadGuard guard();

  if (!find(key, prevs, currs)) {
    return SET_ENTRY_NOT_EXIST;
  } else {
    list_node *node = currs[0];
    // mark upper levles
    for (int64_t i = node->top_level_; i >= 1; --i) {
      list_node *next = node->next_[i];
      list_node *t = NULL;
      while (!is_marked(next)
             && next != (t = __sync_val_compare_and_swap(&node->next_[i], next, mark(next)))) {
        next = t;
      }
    }

    // mark bottom level
    list_node *next = node->next_[0];
    list_node *t = NULL;
    while (true) {
      if (is_marked(next)) {
        return SET_ENTRY_NOT_EXIST;
      } else if (next == (t = __sync_val_compare_and_swap(&node->next_[0], next, mark(next)))) {
        find(key, prevs, currs);

        rcu_read_unlock();
        synchronize_rcu();
        delete node;
        rcu_read_lock();

        break;
      } else {
        next = t;
        // retry
      }
    }
  }

  return 0;
}

template <typename T>
bool OrderedSet<T>::contains(const T &key)
{
  list_node *prev = &head_;
  list_node *curr = NULL;
  list_node *next = NULL;

  RCUReadGuard guard();

  for (int64_t i = MAX_LEVEL; i >= 0; --i) {
    curr = unmark(prev->next_[i]);
    while (true) {
      next = curr->next_[i];
      while (is_marked(next)) {
        curr = unmark(next);
        next = curr->next_[i];
      }

      if (curr != &tail_ && curr->key_ < key) {
        prev = curr;
        curr = next;
      } else {
        break;
      }
    }
  }

  return (curr != &tail_ && curr->key_ == key);
}

template <typename T>
bool OrderedSet<T>::find(const T &key, list_node *prevs[], list_node *currs[])
{
  while (true) {
    list_node *prev = &head_;
    list_node *curr = NULL;
    list_node *next = NULL;
    bool retry = false;

    for (int64_t i = MAX_LEVEL; i >= 0; --i) {
      curr = unmark(prev->next_[i]);
      while (true) {
        next = curr->next_[i];

        while (is_marked(next)) {
          // remove node
          if (__sync_bool_compare_and_swap(&prev->next_[i], curr, unmark(next))) {
            curr = unmark(next);
            next = curr->next_[i];
          } else {
            retry = true;
            break;
          }
        }

        if (retry) break;

        if (curr != &tail_ && curr->key_ < key) {
          prev = curr;
          curr = next;
        } else {
          break;
        }
      }

      if (retry) break;

      prevs[i] = prev;
      currs[i] = curr;
    }

    if (!retry) return (curr != &tail_ && curr->key_ == key);
  }
}

#endif /* _SKIPLIST_H_ */
