#ifndef _LINKED_SET_H_
#define _LINKED_SET_H_

#include "set.h"
#include "linked_list.h"

template <typename T>
class Set
{
public:
  int add(const T &key)
  {
    int list_ret = list_.insert(key);

    if (0 == list_ret) {
      return 0;
    } else if (LIST_ENTRY_DUPLICATE == list_ret) {
      return SET_ENTRY_DUPLICATE;
    } else {
      return SET_ERROR_UNKNOWN;
    }
  }

  int remove(const T &key)
  {
    int list_ret = list_.remove(key);

    if (0 == list_ret) {
      return 0;
    } else if (LIST_ENTRY_NOT_EXSIT == list_ret) {
      return SET_ENTRY_NOT_EXIST;
    } else {
      return SET_ERROR_UNKNOWN;
    }
  }

  bool contains(const T &key)
  {
    return list_.find(key);
  }

  class Iterator {
    friend Set<T>;
  public:
    Iterator &operator ++() { ++iter_; return *this; }
    bool operator != (const Iterator &o) const { return iter_ != o.iter_; }
    T &operator * () { return *iter_; }
  private:
    typename LinkedList<T>::Iterator iter_;
  };

  Iterator begin()
  {
    Iterator iter;
    iter.iter_ = list_.begin();
    return iter;
  }

  Iterator end()
  {
    Iterator iter;
    iter.iter_ = list_.end();
    return iter;
  }

private:
  LinkedList<T> list_;
};

#endif /* _LINKED_SET_H_ */
