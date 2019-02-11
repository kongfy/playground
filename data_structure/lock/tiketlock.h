#ifndef _TIKETLOCK_H_
#define _TIKETLOCK_H_

#include "lock.h"

class TiketLock : public BaseLock
{
public:
  TiketLock() : next_tiket(0), now_serving(0)
  {
  }

  virtual ~TiketLock() override
  {
  }

  virtual int lock() override
  {
    int64_t my_tiket = __sync_fetch_and_add(&next_tiket, 1);
    while (my_tiket != now_serving) {
      asm volatile("pause\n" ::: "memory");
    }
    return 0;
  }

  virtual int unlock() override
  {
    now_serving += 1;
    return 0;
  }
private:
  int64_t next_tiket, now_serving;
};


#endif /* _TIKETLOCK_H_ */
