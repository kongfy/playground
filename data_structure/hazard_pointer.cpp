#include "hazard_pointer.h"

#include <cstring>

HazardManager::HazardManager(int k) : k_(k)
{
  int t = k_ * MAX_THREAD_NUM;
  int c = 0;
  while (t > 0) {
    t >>= 1;
    c++;
  }
  hash_size_ = 1 << (c - 1);
  hash_mask_ = hash_size_ - 1;
}

HazardManager::~HazardManager()
{
  for (int64_t i = 0; i < MAX_THREAD_NUM; ++i) {
    threadlocal &data = storage_[i];
    if (data.rcount > 0) {
      scan(data);
    }
  }
}

bool HazardManager::acquire(const void* const node, const int64_t &tid)
{
  if (tid < 0 || tid >= MAX_THREAD_NUM) {
    return false;
  }

  threadlocal &data = storage_[tid];
  if (data.pcount >= k_) {
    return false;
  }

  data.hp[data.pcount] = node;
  asm volatile("" ::: "memory");
  data.pcount++;
  return true;
}

void HazardManager::release(const void* const node, const int64_t tid)
{
  if (tid < 0 || tid >= MAX_THREAD_NUM) {
    return;
  }

  threadlocal &data = storage_[tid];
  bool found = false;
  for (int64_t i = 0; i < data.pcount && !found; ++i) {
    if (data.hp[i] == node) {
      data.hp[i] = data.hp[data.pcount - 1];
      found = true;
    }
  }

  if (found) {
    asm volatile("" ::: "memory");
    data.pcount--;
  }
}

void HazardManager::retireNode(const void* const node, retire_cb cb)
{
  retireNode(node, get_itid(), cb);
}

void HazardManager::retireNode(const void* const node,
                               const int64_t &tid,
                               retire_cb cb)
{
  if (tid < 0 || tid >= MAX_THREAD_NUM) {
    return;
  }

  threadlocal &data = storage_[tid];
  rnode *new_node = new rnode();
  new_node->node = node;
  new_node->next = data.rlist->next;
  new_node->cb = cb;
  data.rlist->next = new_node;
  data.rcount++;

  if (data.rcount > MAX_THREAD_NUM * k_) {
    scan(data);
  }
}

void HazardManager::scan(threadlocal &rdata)
{
  rnode *p = rdata.rlist;
  rnode *q = p->next;

  bool map[hash_size_];
  memset(map, 0, hash_size_);

  for (int64_t i = 0; i < MAX_THREAD_NUM; ++i) {
    const threadlocal &data = storage_[i];
    for (int64_t j = 0; j < data.pcount; ++j) {
      map[(unsigned long)data.hp[j] & hash_mask_] = true;
    }
  }

  while (q) {
    // scan and free
    if (!map[(unsigned long)q->node & hash_mask_]) {
      const rnode *t = q;
      q = q->next;
      p->next = q;
      rdata.rcount--;
      (*t->cb)(t->node);
      delete t;
    } else {
      p = q;
      q = p->next;
    }
  }
}
