/*
 * This is the source code of SpecNet project
 * It is licensed under MIT License.
 *
 * Copyright (c) Dmitriy Bondarenko
 * feel free to contact me: specnet.messenger@gmail.com
 */

#ifndef FastMemPool_H
#define FastMemPool_H

#include <memory>
#include <atomic>
#include <stdint.h>
#include <algorithm>
#include <string.h>
#include <stdexcept>

#if defined(Debug)
#include <string>
#include <map>
#include <mutex>
#endif

#ifndef DEF_Leaf_Size_Bytes
#define DEF_Leaf_Size_Bytes  65535
#endif
#ifndef DEF_Leaf_Cnt
#define DEF_Leaf_Cnt  16
#endif
#ifndef DEF_Average_Allocation
#define DEF_Average_Allocation  655
#endif
#ifndef DEF_Raise_Exeptions
#define DEF_Raise_Exeptions  true
#endif
#ifndef DEF_Do_OS_malloc
#define DEF_Do_OS_malloc  true
#endif
#if defined(DEF_Auto_deallocate)
#ifndef Debug
#include <set>
#include <mutex>
//#include <iostream>
#endif
#endif


/*
 * FastMemPool
 * Fast thread-safe C++ recycler allocator with memory access control functions.
 * Usage sample example:

 FastMemPool<>  fastMemPool;
 instead of malloc:
 my_array[i]  =  static_cast<int *>(fastMemPool.fmalloc(rand() % 256 + 32));
 my_array[j]  =  static_cast<int *>(FMALLOC(&fastMemPool, (rand() % 256 + 32)));

 instead of free:
 fastMemPool.ffree(my_array[i]);
 FFREE(&fastMemPool, my_array[j]);

 The macros version (FMALLOC, FFREE) is used to provide information about
 the specific location of the error (file, line number, method name).

 Usage examples you can find here:
 https://github.com/DimaBond174/FastMemPool

 * Benefits:
 * 1) Work speed
 * 2) Saving CPU time (can work without mutexes, no atomic waiting cycles)
 * 3) Memory access protection functionality: recognizes its allocations,
 * and also controls buffer overflow, including between its own allocations (while the OS only complains if goes out process memory)
 * 4) Fast deallocation (no cost for additional allocation register structures)
 * 5) If your RAM pool has run out, allocation will be performed through OS malloc,
 * but the functionality (2) of protecting access to memory in full will be preserved.
 * Additionally:
 *  - it is possible to maintain a register of allocations for debugging (included in the code at compile time if defined (Debug))
 *  For example: upon repeated deallocation, in Exception it will tell where the first one happened:
 * "FastMemPool::ffreed: this pointer has already been freed from: test_exe.cpp, at 9  line, in free1"
 *
*/
template<int Leaf_Size_Bytes = DEF_Leaf_Size_Bytes, int Leaf_Cnt = DEF_Leaf_Cnt,
  int Average_Allocation = DEF_Average_Allocation, bool Do_OS_malloc = DEF_Do_OS_malloc,
  bool Raise_Exeptions = DEF_Raise_Exeptions>
class FastMemPool
{
public:
  /**
   * @brief fmalloc
   * Allocation function instead of malloc
   * @param allocation_size  -  volume to allocate
   * @return - allocation ptr
   */
  void  * fmalloc(std::size_t  allocation_size)
  {
    // Allocation will include a header with service information:
    const int  real_size = allocation_size  +  sizeof(AllocHeader);
    // Starting leaf for finding the allocation place:
    const int start_leaf = cur_leaf.load(std::memory_order_relaxed);
    // Selected leaf identifier:
    int leaf_id = start_leaf;
    // Resulting allocation:
    char  *re  =  nullptr;

    /*
      Exit the loop at the end of the loop when we meet start_leaf again.
      If it is impossible to make an allocation in your own memory pool,
      an escalation to OS malloc will occur, but the access control functionality will remain operational.
   */
    do {
      const int available  =  leaf_array[leaf_id].available.load(std::memory_order_acquire);
      if (available  >=  real_size)
      {
        // we reserve memory (the buffer is distributed from the end with a bite):
        const int  available_after  =  leaf_array[leaf_id].available.fetch_sub(real_size, std::memory_order_acq_rel)  -  real_size;
        // and if successful, a positive number should have returned,
        // otherwise we would have broken through the bottom of the buffer:
        if (available_after >= 0)
        {  // the resulting distribution address is easy to obtain, because it starts immediately
          // after "available", since addressing from &[0] then this is "buf + available":
          re  =  leaf_array[leaf_id].buf + available_after;
          if (available_after < Average_Allocation)
          {  // Let's tell the rest of the threads to use a different memory page:
            const int next_id = start_leaf + 1;
            if (next_id >= Leaf_Cnt)
            {
              cur_leaf.store(0, std::memory_order_release);
            } else {
              cur_leaf.store(next_id, std::memory_order_release);
            }
          }
          break; // finished the search, return the allocation pointer
        }
      }
      ++leaf_id;
      if (Leaf_Cnt == leaf_id)  {  leaf_id  =  0;  }
    } while (leaf_id  !=  start_leaf);

    bool  do_OS_malloc  =  !re;
    if  (do_OS_malloc)
    {  // Now the escalation to OS malloc will occur:
      if (Do_OS_malloc)
      {
        // Паттерн "Chain of responsibility" в действии:
        re = static_cast<char*>(malloc(real_size));
#if defined(DEF_Auto_deallocate)
   #if not defined(Debug)
        if (re) {
          std::lock_guard<std::mutex>  lg(mut_set_alloc_info);
          set_alloc_info.emplace(re);
        }
  #endif
#endif
      } else {
        if constexpr (Raise_Exeptions)
        {
          throw std::range_error("FastMemPool::fmalloc: need do_OS_malloc, but it disabled");
        }
      }
    }

    if (re)
    { // if the allocation was successful, then fill in the header:
      AllocHeader  *head  =  reinterpret_cast<AllocHeader  *>(re);
      if (do_OS_malloc)
      {
        head->leaf_id  =  OS_malloc_id;
        head->tag_this =  TAG_OS_malloc;
      } else {
        head->leaf_id  =  leaf_id;
        head->tag_this = ((uint64_t)this) + leaf_id;
      }
      head->size  =  allocation_size;
      return  (re + sizeof(AllocHeader));
    }

    return  nullptr;
  }  // fmalloc


  /**
   * @brief ffree  -  function to release allocation instead of "free"
   * @param ptr  -  allocation pointer obtained earlier via fmaloc
   */
  void  ffree(void  *ptr)
  {
    // Rewind back to get the AllocHeader:
    char  *to_free  =  static_cast<char  *>(ptr)  -  sizeof(AllocHeader);
    AllocHeader  *head  =  reinterpret_cast<AllocHeader  *>(to_free);
    if  (0 <= head->size  &&  head->size < Leaf_Size_Bytes
         &&  0 <= head->leaf_id  &&  head->leaf_id < Leaf_Cnt
         && ((uint64_t)this) == (head->tag_this - head->leaf_id)
         &&  leaf_array[head->leaf_id].buf)
    {  //  ok this is my allocation
      const int  real_size = head->size  +  sizeof(AllocHeader);
      const int  deallocated  =  leaf_array[head->leaf_id].deallocated.fetch_add(real_size, std::memory_order_acq_rel)  +  real_size;
      int  available  =  leaf_array[head->leaf_id].available.load(std::memory_order_acquire);
      if (deallocated  == (Leaf_Size_Bytes - available))
      {  // everything that was allocated is now returned, we will try, carefully, reset the Leaf
        if (leaf_array[head->leaf_id].available.compare_exchange_strong(available,  Leaf_Size_Bytes))
        {
          leaf_array[head->leaf_id].deallocated  -=  deallocated;
        }
      }

      // Cleanup so that unique TAG_my_alloc will be keep unique in RAM:
      memset(head,  0,  sizeof(AllocHeader));
    }  else if (TAG_OS_malloc  ==  head->tag_this
         &&  OS_malloc_id  ==  head->leaf_id
         &&  head->size > 0   )
    {  // ok, это OS malloc
      // Cleanup so that unique TAG_my_alloc will be keep unique in RAM:
      memset(head,  0,  sizeof(AllocHeader));
#if defined(DEF_Auto_deallocate)
   #if not defined(Debug)
        {
          std::lock_guard<std::mutex>  lg(mut_set_alloc_info);
          set_alloc_info.erase(head);
        }
  #endif
#endif
      free(head);
    }  else  {
      // this is someone else's allocation, Exception
      if constexpr (Raise_Exeptions)
      {
          throw std::range_error("FastMemPool::ffree: this is someone else's allocation");
      }
    }
    return;
  }

  /**
   * @brief check_access  -  checking the accessibility of the target memory area
   * @param base_alloc_ptr - the assumed address of the base allocation from FastMemPool
   * @param target_ptr  -  start of target memory area
   * @param target_size  -  the size of the structure to access
   * @return - true if the target memory area belongs to the base allocation from FastMemPool
   * Advantages: allows you to determine whether you have climbed into another OWN allocation,
   * while the OS swears only if it climbed out of process RAM
   */
  bool  check_access(void  *base_alloc_ptr,  void  *target_ptr,  std::size_t  target_size)
  {
    bool  re  = false;
    AllocHeader  *head  =  reinterpret_cast<AllocHeader  *>(static_cast<char  *>(base_alloc_ptr)  -  sizeof(AllocHeader));
    if  (0 <= head->size  &&  head->size < Leaf_Size_Bytes
         &&  0 <= head->leaf_id  &&  head->leaf_id < Leaf_Cnt
         && ((uint64_t)this) == (head->tag_this - head->leaf_id))
    {  //  ok, this is FastMemPool allocation
      char  *start  =  static_cast<char  *>(base_alloc_ptr);
      char  *end  =  start  +  head->size;
      char  *buf  =  leaf_array[head->leaf_id].buf;
      if (buf  &&  buf  <=  start  &&  (buf  +  Leaf_Size_Bytes) >= end)
      { // Let's check whether it has gone beyond the allocation limits:
        char  *target_start  =  static_cast<char  *>(target_ptr);
        char  *target_end  =  target_start  +  target_size;
        if  (start  <=  target_start  &&  target_end <= end)
        {
          re = true;
        }  else  {
          if constexpr (Raise_Exeptions)
          {
              throw std::range_error("FastMemPool::check_access: out of allocation");
          }
        } // elseif  (start  >  target_start
      }  else {
        if constexpr (Raise_Exeptions)
        {
            throw std::range_error("FastMemPool::check_access: out of Leaf");
        }
      } // elseif (!buf
    }  else  if ( TAG_OS_malloc  ==  head->tag_this
         &&  OS_malloc_id  ==  head->leaf_id
         &&  head->size > 0 )
    {
      // Let's check whether it has gone beyond the allocation limits:
      char  *start  =  static_cast<char  *>(base_alloc_ptr);
      char  *end  =  start  +  head->size;
      char  *target_start  =  static_cast<char  *>(target_ptr);
      char  *target_end  =  target_start  +  target_size;
      if  (start  <=  target_start  &&  target_end <= end)
      {
        re = true;
      }  else  {
        if constexpr (Raise_Exeptions)
        {
            throw std::range_error("FastMemPool::check_access: out of OS malloc allocation");
        }
      } // elseif  (start  >  target_start

    }  else  {
      if constexpr (Raise_Exeptions)
      {
          throw std::range_error("FastMemPool::check_access: not  FastMemPool's allocation");
      }
    }
    return  re;
  } // check_access

  /**
   * @brief FastMemPool - construct
   */
  FastMemPool()  noexcept
  {
    void  *buf_array[Leaf_Cnt];
    for (int   i  =  0;  i  < Leaf_Cnt ;  ++i)
    {
      buf_array[i]  =  malloc(Leaf_Size_Bytes);
    }
    std::sort(std::begin(buf_array), std::end(buf_array), [](const void * lh, const void * rh) { return (uint64_t)(lh) < (uint64_t)(rh); });
    uint64_t last = 0;
    for (int   i  =  0;  i  < Leaf_Cnt ;  ++i)
    {
      if (buf_array[i])
      {
        leaf_array[i].buf = static_cast<char *>(buf_array[i]);
        leaf_array[i].available.store(Leaf_Size_Bytes,  std::memory_order_relaxed);
        leaf_array[i].deallocated.store(0,  std::memory_order_relaxed);
      }  else  {
        leaf_array[i].buf = nullptr;
        leaf_array[i].available.store(0,  std::memory_order_relaxed);
        leaf_array[i].deallocated.store(Leaf_Size_Bytes,  std::memory_order_relaxed);
      }
    }
    std::atomic_thread_fence(std::memory_order_release);
    return;
  }  // FastMemPool

  /**
   * @brief instance - Singleton implementation, if anyone needs it all of a sudden
   * @return
    IMPORTANT: we rely on the linker to be able to remove duplicates of this method from all
    translation units .. provided the template parameters are the same
   */
  static FastMemPool * instance() {
    static FastMemPool  fastMemPool;
    return &fastMemPool;
  }

  ~FastMemPool()
  {
    for (int   i  =  0;  i  < Leaf_Cnt ;  ++i)
    {
      if (leaf_array[i].buf)  free(leaf_array[i].buf);
    }
#if defined(DEF_Auto_deallocate)
    #if defined(Debug)
    {
      std::lock_guard<std::mutex>  lg(mut_map_alloc_info);
      for (auto &&it : map_alloc_info)
      {
        free(it.first);
      }
      map_alloc_info.clear();
    }
    #else
    {
      std::lock_guard<std::mutex>  lg(mut_set_alloc_info);
      for (auto &&it : set_alloc_info)
      {
        //std::cout << " auto deallocated :" << (uint64_t(it)) << std::endl;
        free(it);
      }
      set_alloc_info.clear();
    }
    #endif
#endif
    return;
  }

  FastMemPool &operator=(const FastMemPool &) = delete;
  FastMemPool (const FastMemPool &) = delete;

  FastMemPool &operator=(FastMemPool &&) = delete;
  FastMemPool (FastMemPool &&) = delete;

#if defined(Debug)
  /**
   * @brief fmallocd
   * Decorator for fmalloc method - stores information about the location of the allocation
   * @param filename
   * @param line
   * @param function_name
   * @param allocation_size
   * @return
   */
  void  * fmallocd(const char *filename, unsigned int line, const char *function_name,  std::size_t  allocation_size)
  {
    void  *re  =  fmalloc(allocation_size);
    if (re)
    {
      std::lock_guard<std::mutex>  lg(mut_map_alloc_info);
      auto it = map_alloc_info.try_emplace(re,  AllocInfo());
      if (it.first->second.allocated)  {
        std::string err("FastMemPool::fmallocd: already allocated by ");
        err.append(it.first->second.who);
        throw std::range_error(err);
      }  else  {
        auto &&who = it.first->second.who;
        who.clear();
        who.append(filename).append(", at ")
            .append(std::to_string(line)).append("  line, in ").append(function_name);
        it.first->second.allocated = true;
      }

    }
    return  re;
  }

  /**
   * @brief ffreed
   * Decorator for the free method
   * - saves information about the place of deallocation
   * - in case of re-deallocation, tells where the first deallocation took place
   * @param filename
   * @param line
   * @param function_name
   * @param ptr
   */
  void  ffreed(const char *filename, unsigned int line, const char *function_name,  void  *ptr)
  {
    if (ptr)
    {
      {
        std::lock_guard<std::mutex>  lg(mut_map_alloc_info);
        auto it = map_alloc_info.find(ptr);
        if (map_alloc_info.end() == it)
        {
          throw std::range_error("FastMemPool::ffreed: this pointer has never been allocated");
        }
        if (!it->second.allocated)
        {
          std::string err("FastMemPool::ffreed: this pointer has already been freed from: ");
          err.append(it->second.who);
          throw std::range_error(err);
        }
        auto &&who = it->second.who;
        who.clear();
        who.append(filename).append(", at ")
            .append(std::to_string(line)).append("  line, in ").append(function_name);
        it->second.allocated = false;
      }
      ffree(ptr);
    }
    return;
  } // ffreed

  bool  check_accessd(const char *filename, unsigned int line, const char *function_name,  void  *base_alloc_ptr,  void  *target_ptr,  std::size_t  target_size)
  {
    bool  re   =  false ;
    try {
      re  =  check_access(base_alloc_ptr,  target_ptr,  target_size);
    } catch (std::range_error e) {

    }

    if (!re)
    {
      std::string who("FastMemPool::check_accessd buffer overflow at ");
      who.append(filename).append(", at ")
          .append(std::to_string(line)).append("  line, in ").append(function_name);
      if constexpr (Raise_Exeptions)
      {
          throw std::range_error(who);
      }
    }
    return  re;
  } // check_accessd

#endif
private:

  struct Leaf
  {
      char  *buf;
      // available == offset
      std::atomic<int>  available  {  Leaf_Size_Bytes  };
      // control of deallocations:
      std::atomic<int>  deallocated  {  0  };
  };

  /*
   * AllocHeader
    Allows you to quickly assess whether your allocation and where:
    tag_this - a unique number unlikely to occur in RAM,
       while there is a mutual check with leaf_id:
      if (tag_this - leaf_id)  !=  this  and  OS malloc != -OS_malloc_id,  then this is someone else's allocation
    leaf_id == {  -2020071708  } => allocation was done in OS malloc,
    leaf_id >= 0 => allocation in leaf_array[leaf_id],
    size - allocation size, if 0 <= size> = LeafSizeBytes - it means someone else's allocation.
     The header can be obtained at any time by a negative offset relative to the *pointer.
*/
  const int  OS_malloc_id  {  -2020071708  };
  const int  TAG_OS_malloc  {  1020071708  };
  struct AllocHeader {
    /*
     label of own allocations:
     tag_this = (uint64_t)this + leaf_id */
    uint64_t  tag_this  {  2020071700  };
    // allocation size (without sizeof(AllocHeader)):
    int  size;
    // allocation place id (Leaf ID  or OS_malloc_id):
    int  leaf_id  {  -2020071708  };
  };

  // Memory pool:
  Leaf  leaf_array[Leaf_Cnt];
  std::atomic<int>  cur_leaf  {  0  };

#if defined(Debug)
  struct AllocInfo {
    std::string  who;  // who performed the operation: file, code line number, in which method
    bool  allocated  {  false  };  // true - allocated, false - deallocated
  };
  std::map<void *,  AllocInfo>  map_alloc_info;
  std::mutex  mut_map_alloc_info;

  // Just for easy viewing in debug:
  int DLeaf_Size_Bytes  { Leaf_Size_Bytes };
  int DLeaf_Cnt { Leaf_Cnt };
  int DAverage_Allocation { Average_Allocation };
  bool DDo_OS_malloc{ Do_OS_malloc };
  bool DRaise_Exeptions { Raise_Exeptions };
#else
  #if defined(DEF_Auto_deallocate)
  std::set<void *>  set_alloc_info;
  std::mutex  mut_set_alloc_info;
  #endif
#endif
};


/**
   * @brief FMALLOC
   * Allocation function instead of malloc
   * @param iFastMemPool  -  an instance of FastMemPool in which we allocate
   * @param allocation_size  -  volume to allocate
   * @return - allocation ptr
*/
#if defined(Debug)
#define FMALLOC(iFastMemPool, allocation_size) \
   (iFastMemPool)->fmallocd (__FILE__, __LINE__, __FUNCTION__, allocation_size)
#else
#define FMALLOC(iFastMemPool, allocation_size) \
   (iFastMemPool)->fmalloc (allocation_size)
#endif

/**
 * @brief FFREE  -  function to release allocation instead of "free"
 * @param iFastMemPool  - an instance of FastMemPool in which we allocate
 * @param ptr  -  allocation pointer obtained earlier via fmaloc
 */
#if defined(Debug)
#define FFREE(iFastMemPool, ptr) \
   (iFastMemPool)->ffreed (__FILE__, __LINE__, __FUNCTION__, ptr)
#else
#define FFREE(iFastMemPool, ptr) \
   (iFastMemPool)->ffree (ptr)
#endif

/**
   * @brief FCHECK_ACCESS  -  checking the accessibility of the target memory area
   * @param iFastMemPool  - an instance of FastMemPool in which we allocate
   * @param base_alloc_ptr - the assumed address of the base allocation from FastMemPool
   * @param target_ptr  -  start of target memory area
   * @param target_size  -  the size of the structure to access
   * @return - true if the target memory area belongs to the base allocation from FastMemPool
   * Advantages: allows you to determine whether you have climbed into another OWN allocation,
   * while the OS swears only if it climbed out of process RAM
 */
#if defined(Debug)
#define FCHECK_ACCESS(iFastMemPool, base_alloc_ptr, target_ptr, target_size) \
   (iFastMemPool)->check_accessd (__FILE__, __LINE__, __FUNCTION__, base_alloc_ptr,  target_ptr,  target_size)
#else
#define FCHECK_ACCESS(iFastMemPool, base_alloc_ptr, target_ptr, target_size) \
   (iFastMemPool)->check_access (base_alloc_ptr,  target_ptr,  target_size)
#endif

struct FastMemPoolNull
{
  // Null object
};

/**
 * FastMemPoolAllocator
 * == std::allocator<T> template
 * Default works with SingleTone FastMemPool<>::instance()
 * FastMemPool can be injected as template method or allocation strategy:

 // inject template (Template method):
 FastMemPoolAllocator<std::string, FastMemPool<111, 11> > myAllocator;

 // inject instance (Strategy):
  using MyAllocatorType = FastMemPool<333, 33>;
  MyAllocatorType  fastMemPool;  // instance of
  FastMemPoolAllocator<std::string, MyAllocatorType > myAllocator(&fastMemPool);
 */
template<class T, class FAllocator = FastMemPoolNull >
struct FastMemPoolAllocator : public std::allocator<T>  {
  typedef T value_type;
  using  MyAllocatorType = FAllocator;
  MyAllocatorType * p_allocator  {  nullptr  };
  FastMemPoolAllocator() = default;
  FastMemPoolAllocator(MyAllocatorType  *in_allocator) : p_allocator(in_allocator) { }
  template <class U> constexpr FastMemPoolAllocator (const FastMemPoolAllocator  <U>&)
  noexcept {}

  T* allocate(std::size_t n) {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof (T))
      throw std::bad_alloc();
    if constexpr(std::is_same<FAllocator, FastMemPoolNull>::value)
    {
      if (auto p = static_cast<T *>(FMALLOC(FastMemPool<>::instance(), (n * sizeof (T)))))
        return p;
    }  else {
      if (p_allocator) {
        if (auto p = static_cast<T *>(FMALLOC(p_allocator, (n * sizeof (T)))))
          return p;
      } else {
        if (auto p = static_cast<T *>(FMALLOC(FAllocator::instance(), (n * sizeof (T)))))
          return p;
      }

    }

    throw  std::bad_alloc();
  } // alloc

  void deallocate(T* p,  std::size_t) noexcept
  {
    if constexpr(std::is_same<FAllocator, FastMemPoolNull>::value)
    {
      FFREE(FastMemPool<>::instance(),  p);
    }  else {
      if (p_allocator) {
        FFREE(p_allocator,  p);
      } else {
        FFREE(FAllocator::instance(),  p);
      }
    }
    return;
  }

  template<typename _Up, typename... _Args>
  void
  construct(_Up* __p, _Args&&... __args)
  { ::new((void *)__p) _Up(std::forward<_Args>(__args)...); }

  template<typename _Up>
  void
  destroy(_Up* __p) { __p->~_Up(); }
};
template <class T, class U>
bool operator==(const FastMemPoolAllocator<T>&, const FastMemPoolAllocator<U>&) { return true; }
template<class T, class U>
bool operator!=(const FastMemPoolAllocator<T>&, const FastMemPoolAllocator<U>&) { return false; }

#endif //FastMemPool_H
