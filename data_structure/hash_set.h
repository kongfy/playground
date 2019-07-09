#ifndef _HASH_SET_H_
#define _HASH_SET_H_

#include <string.h>
#include <functional>

#include "set.h"
#include "linked_list.h"

template <typename T, typename Hash = std::hash<T> >
class HashSet
{
private:
  class HashNode
  {
  public:
    HashNode() : key_(), list_key_(0) {}

    explicit HashNode(const T &key) : key_(key), list_key_(0)
    {
      uint64_t hash_val = hash_func_(key_);
      list_key_ = reverse(hash_val) | 1LL;
    }

    HashNode(const HashNode &o) : key_(o.key_), list_key_(o.list_key_) {}

    bool is_sentinel() const
    {
      return 0L == (list_key_ & 1LL);
    }

    bool operator ==(const HashNode &o) const
    {
      return list_key_ == o.list_key_ && key_ == o.key_;
    }

    bool operator >(const HashNode &o) const
    {
      return list_key_ > o.list_key_
        || (list_key_ == o.list_key_ && key_ > o.key_);
    }

    bool operator <(const HashNode &o) const
    {
      return list_key_ < o.list_key_
        || (list_key_ == o.list_key_ && key_ < o.key_);
    }

    bool operator !=(const HashNode &o) const { return !(*this == o); }
    bool operator >=(const HashNode &o) const { return *this > o || *this == o; }
    bool operator <=(const HashNode &o) const { return *this < o || *this == o; }

    static uint64_t reverse(uint64_t x)
    {
      x = (((x & 0xaaaaaaaaaaaaaaaaULL) >> 1) | ((x & 0x5555555555555555ULL) << 1));
      x = (((x & 0xccccccccccccccccULL) >> 2) | ((x & 0x3333333333333333ULL) << 2));
      x = (((x & 0xf0f0f0f0f0f0f0f0ULL) >> 4) | ((x & 0x0f0f0f0f0f0f0f0fULL) << 4));
      x = (((x & 0xff00ff00ff00ff00ULL) >> 8) | ((x & 0x00ff00ff00ff00ffULL) << 8));
      x = (((x & 0xffff0000ffff0000ULL) >> 16) | ((x & 0x0000ffff0000ffffULL) << 16));
      return ((x >> 32) | (x << 32));
    }

    T key_;
    uint64_t list_key_;
  };

  class Sentinel : public HashNode
  {
  public:
    Sentinel(const int64_t idx) : HashNode()
    {
      HashNode::list_key_ = HashNode::reverse(idx) & ~1ULL;
    }
  };

  typedef LinkedList<HashNode> BucketList;

public:
  HashSet() : bucket_size_(2), set_size_(0), buckets_()
  {
    memset(buckets_, 0, sizeof(buckets_));
    buckets_[0] = new BucketList();
  }

  ~HashSet() {} // TODO

  int add(const T &key)
  {
    BucketList *bucket = getBucket(key);
    HashNode node(key);
    // emmm... 为了接口清晰，hash计算了两次

    int list_ret = bucket->insert(node);

    if (0 == list_ret) {
      // resize
      int64_t set_size = __sync_add_and_fetch(&set_size_, 1LL);
      int64_t bucket_size = bucket_size_;
      if ((double)set_size / bucket_size > 0.3) { // TODO: improve the condition
        int64_t new_bucket_size = bucket_size << 1LL;
        if (new_bucket_size > MAX_BUCKET_NUM) {
        } else {
          __sync_bool_compare_and_swap(&bucket_size_, bucket_size, new_bucket_size);
        }
      }
    } else if (LIST_ENTRY_DUPLICATE == list_ret) {
      return SET_ENTRY_DUPLICATE;
    } else {
      return SET_ERROR_UNKNOWN;
    }

    return 0;
  }

  // TODO: remove()

  bool contains(const T &key)
  {
    BucketList *bucket = getBucket(key);

    // iterate all node in this bucket
    for (typename BucketList::Iterator iter = bucket->begin(); iter != bucket->end(); ++iter) {
      if ((*iter).is_sentinel()) {
        break;
      } else if ((*iter).key_ == key) {
        return true;
      }
    }

    return false;
  }

  class Iterator {
    friend HashSet<T, Hash>;
  public:
    Iterator &operator ++()
    {
      do {
        ++iter_;
      } while (iter_ != end_ && (*iter_).is_sentinel());

      return *this;
    }

    bool operator != (const Iterator &o) const { return iter_ != o.iter_; }
    T &operator * () { return (*iter_).key_; }
  private:
    typename BucketList::Iterator iter_;
    typename BucketList::Iterator end_;
  };

  Iterator begin()
  {
    Iterator iter;
    if (buckets_[0]) {
      iter.iter_ = buckets_[0]->begin();
      iter.end_ = buckets_[0]->end();
    }
    return iter;
  }

  Iterator end()
  {
    Iterator iter;
    if (buckets_[0]) {
      iter.iter_ = buckets_[0]->end();
      iter.end_ = buckets_[0]->end();
    }
    return iter;
  }

private:

  int getParent(const int index)
  {
    // 去掉最高的非0bit
    int64_t t = bucket_size_;
    while (t > index) t >>= 1LL;
    return index - t;
  }

  void initBucket(const int64_t index)
  {
    int parent = getParent(index);
    if (!buckets_[parent]) {
      initBucket(parent);
    }

    Sentinel sentinel(index);
    BucketList *bucket = BucketList::shortcut(*buckets_[parent], sentinel);

    if (NULL != bucket) {
      if (!__sync_bool_compare_and_swap(&buckets_[index], NULL, bucket)) {
        delete bucket;
      }
    } else {
      abort();
    }
  }

  BucketList *getBucket(const T &key)
  {
    uint64_t hash_val = hash_func_(key);
    int64_t index = hash_val % bucket_size_;

    if (NULL == buckets_[index]) {
      initBucket(index);
    }

    return buckets_[index];
  }

  static const int64_t MAX_BUCKET_NUM = 1LL << 20LL; // 1048576
  // TODO: tree structure buckets

  static Hash hash_func_;
  volatile int64_t bucket_size_;
  int64_t set_size_;
  BucketList *buckets_[MAX_BUCKET_NUM];
};


#endif /* _HASH_SET_H_ */
