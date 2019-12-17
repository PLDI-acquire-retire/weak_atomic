
#ifndef REF_COUNTING_H
#define REF_COUNTING_H

#include <atomic>

#include <atomic_ref_cnt/acquire_retire_waitfree.hpp>
#include <atomic_ref_cnt/utils.hpp>

template <typename T, template<typename> typename AcquireRetire = AcquireRetireWaitfree>
struct ref_ptr {
  public:
    ref_ptr() : p(nullptr) {}
    ref_ptr(T* p) : p(p) {
      if(p) p->add_counter(1); 
    }

    // copy pointer from b into new ref_ptr
    ref_ptr(ref_ptr& b) { 
      p = ar.slots[utils::my_id].acquire(&(b.p)); 
      if (p) p->add_counter(1); 
      ar.slots[utils::my_id].release();
    }

    // update this ref_ptr with new ref_ptr b
    void update(ref_ptr b) { 
      T* old = utils::FAS(&p, b.p);
      ar.slots[utils::my_id].retire(old);
      b.p = nullptr;
      eject_and_decrement(utils::my_id);
    }

    template<typename R, typename F> 
    R with_ptr(F f) { 
      T* ptr = ar.slots[utils::my_id].acquire(&p);
      if constexpr (std::is_same<R, void>::value) {
        f(ptr); 
        ar.slots[utils::my_id].release();
      } else {
        R r = f(ptr); 
        ar.slots[utils::my_id].release();
        return r;         
      }
    }

    ~ref_ptr() { decrement(p); }

    // Calls eject_all() for all processes and destructs everything that gets returned
    // Only call this during a quiescent state 
    // Important for cleaning up lingering items in the retired list
    static void clear_all() {
      for(int tid = 0; tid < MAX_THREADS; tid++) {
        for(T* t : ar.slots[tid].eject_all())  
          decrement(t);
        while(eject_and_decrement(tid)) { }    
      }
    }

  private:
    T* p;

    static AcquireRetire<T*> ar;

    static void decrement(T* o) {
      if (o != nullptr && o->add_counter(-1) == 1) 
        delete o;
    } 

    static bool eject_and_decrement(int tid) {
      std::optional<T*> t = ar.slots[tid].eject();
      if(t) {
        decrement(t.value());
        return true;
      } 
      return false;
    }
};

template<typename T, template<typename> typename AcquireRetire>
AcquireRetire<T*> ref_ptr<T, AcquireRetire>::ar{MAX_THREADS, nullptr};

#endif // REF_COUNTING_H