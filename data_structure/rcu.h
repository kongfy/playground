#ifndef _RCU_H_
#define _RCU_H_

void rcu_read_lock();
void rcu_read_unlock();
void synchronize_rcu();

typedef void (*rcu_cb)(const void* const p);
void call_rcu(const void* const p, rcu_cb cb);

class RCUReadGuard
{
public:
  RCUReadGuard() { rcu_read_lock(); }
  ~RCUReadGuard() { rcu_read_unlock(); }
};

#endif /* _RCU_H_ */
