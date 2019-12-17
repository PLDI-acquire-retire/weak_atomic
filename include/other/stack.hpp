
#ifndef STACK_H
#define STACK_H

#include <atomic>
#include <atomic_ref_cnt/utils.hpp>
#include "memory_reclamation.hpp"

template<typename T>
struct Stack { 

private:
  struct Node { T value; Node* next; };
  struct Node *head;
  MemoryManager<Node> mm;

public:
  Stack() : head(nullptr) {}
  ~Stack() {
    while(head != nullptr){
      Node* tmp = head;
      head = head->next;
      delete tmp;
    }
  }
  
  std::optional<T> peek() {
    return mm.template protected_read<std::optional<T> >(&head, [] (Node* p) {
       if (p == nullptr) return std::optional<T>();
       else return std::optional<T>(p->value);
     });
  }

  void push(T v) {
    Node* a = new Node();
    a->value = v;
    auto try_push = [&] (Node* p) {
      a->next = p;
      return utils::CAS(&head, p, a);};
    while (!mm.template protected_read<bool>(&head,try_push)) {};
  }

  std::optional<T> pop() {
    Node* r;
    auto try_pop = [&] (Node* p) {
      r = p;
      if (p == nullptr) return true;
      return utils::CAS(&head, p, p->next);};
    while (!mm.template protected_read<bool>(&head, try_pop)) {};
    if (r == nullptr) return std::optional<T>();
    T a = r->value;
    mm.safe_free(r);
    return std::optional<T>(a);
  } 
};

#endif // STACK_H