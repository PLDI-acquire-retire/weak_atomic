
#include<atomic_ref_cnt/weak_atomic.hpp>

template<typename T>
struct WeakAtomicSharedPtr {
  weak_atomic<shared_ptr<T> > ptr;

  WeakAtomicSharedPtr(WeakAtomicSharedPtr& _x) : ptr(_x.ptr.load()) {};
  T* load() { return ptr.load().get(); }
  void store(T* p) { ptr.store(shared_ptr<T>(p)); }
}