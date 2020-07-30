/*
 * This is the source code of SpecNet project
 * It is licensed under MIT License.
 *
 * Copyright (c) Dmitriy Bondarenko
 * feel free to contact me: specnet.messenger@gmail.com
 */

#ifndef SPECSTACK_H
#define SPECSTACK_H

#include <atomic>

class  IStack  {
 public:
  IStack  *nextIStack;
};

template  <typename  T  =  IStack>
class  SpecStack  {
 public:
  bool not_empty()  {
    if  (head)  {  return true;  }
    return false;
  }

  void  push(T * node)  {
    node->nextIStack  =  head;
    head  =  node;
  }

  T * pop()  {
    if  (head)  {
      T  *re  =  head;
      head  =  re->nextIStack;
      return re;
    }
    return nullptr;
  }

  void  swap(SpecStack  &other) {
    T  *re  =  head;
    head  =  other.head;
    other.head  =  re;
    return;
  }

  T * swap(T  *newHead)  {
    T  *re  =  head;
    head  =  newHead;
    return  re;
  }

  T * getFirst()  {
    return   head;
  }

 private:
  T  *head  {  nullptr  };
};

template <typename T = IStack>
class SpecSafeStack {
    std::atomic<T*> head {nullptr};
public:
    void push(T * node)  {
        node->nextIStack=head.load();
        while(!head.compare_exchange_weak(node->nextIStack, node)){}
    }

    T* pop() {
      T* re  =  head.load();
      while (re  &&  !head.compare_exchange_weak(re, re->nextIStack)) { }
      return re;
    }

    T * getStack() {
        return head.exchange(nullptr, std::memory_order_acq_rel);
    }

    void moveTo(SpecStack<T> &to) {
      T *h = getStack();
      while (h)
      {
        T *next  =  h->nextIStack;
        to.push(h);
        h  =  next;
      }
      return;
    }

    T * getReverse() {
      SpecStack<T> from;
      from.swap(head.exchange(nullptr, std::memory_order_acq_rel));
      SpecStack<T> to;
      while (T *ptr = from.pop())
      {
        to.push(ptr);
      }
       return to.swap(nullptr) ;
    } // getList

    void getReverse(SpecStack<T> &to) {
      SpecStack<T> from;
      from.swap(head.exchange(nullptr, std::memory_order_acq_rel));
      while (T *ptr = from.pop())
      {
        to.push(ptr);
      }
       return;
    } // getReverse

    T * swap(T * newHead){
        return head.exchange(newHead, std::memory_order_acq_rel);
    }
};

template  <typename  T  =  IStack,  int  N  =  100>
class  SpecStackPool  {
 public:
  SpecStackPool() { cnt  =  0; }
  ~SpecStackPool() {
    T  *p;
    while ((p = stack.pop())) { delete  p; }
  }

  T  *get() {
    T  *re  =  stack.pop();
    if (re)  {
      --cnt;
      re->clear();
    }  else  {  re  =  new T();  }
    return  re;
  }

  template<typename D>
  T  *get(D  body) {
    T  *re  =  stack.pop();
    if (re)  {
      --cnt;
      re->p_body  =  body;
    }  else  {  re  =  new T(body);  }
    return  re;
  }

  void  recycle(T  *item)  {
    if (cnt >= N)  {
      delete  item;
    }  else  {
      stack.push(item);
      ++cnt;
    }
  }

 private:
  int  cnt;
  SpecStack<T>  stack;
};

template  <typename  T  =  IStack>
class  SpecQueue  {
 public:
  bool not_empty()  {
    if  (head)  {  return true;  }
    return false;
  }

  void  add_head(T  *node)  {
    node->nextIStack  =  head;
    if (head)  {
      head->nextIStack  =  node;          
    }  else  {
      tail  =  node;
    }
    head  =  node;
  }

  T * pop_tail()  {
    T  *re  =  tail;
    if (re)  {
      tail  =  re->nextIStack;
      if (!tail)  {
        head  =  nullptr;
      }
    }
    return re;
  }

 private:
  T  *head  {  nullptr  };
  T  *tail  {  nullptr  };
};

#endif // SPECSTACK_H
