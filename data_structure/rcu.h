#ifndef _RCU_H_
#define _RCU_H_

class RCUMgr
{
public:
  typedef void (*rcu_cb)(const void* const p);

  virtual void rcu_read_lock() = 0;
  virtual void rcu_read_unlock() = 0;
  virtual void synchronize_rcu() = 0;
  virtual void call_rcu(const void* const p, rcu_cb cb) = 0;

  RCUMgr() {};
  virtual ~RCUMgr() {};
  RCUMgr(const RCUMgr&) = delete;
  RCUMgr &operator=(const RCUMgr&) = delete;
};

class RCUReadGuard
{
public:
  RCUReadGuard(RCUMgr &rcu) : rcu_(rcu) { rcu_.rcu_read_lock(); }
  ~RCUReadGuard() { rcu_.rcu_read_unlock(); }
private:
  RCUMgr &rcu_;
};

#endif /* _RCU_H_ */
