/*
 * This is the source code of SpecNet project
 * It is licensed under MIT License.
 *
 * Copyright (c) Dmitriy Bondarenko
 * feel free to contact me: specnet.messenger@gmail.com
 */

#ifndef MemPool_H
#define MemPool_H

#include <memory>
#include <stdint.h>
#include <algorithm>
#include <string.h>
#include <stdexcept>
#include <map>

/*
 * Single threaded MemPool for test only
*/
template<int Leaf_Size_Bytes = 1024, int Leaf_Cnt = 16,
  int Average_Allocation = 512, bool Do_OS_malloc = true,
  bool Raise_Exeptions = true>
class MemPool
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
    const int start_leaf = cur_leaf;
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
      const int available  =  leaf_array[leaf_id].available;
      if (available  >=  real_size)
      {
        // we reserve memory (the buffer is distributed from the end with a bite):
        const int  available_after  =  available  -  real_size;
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
              cur_leaf  =  0;
            } else {
              cur_leaf  =  next_id;
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
          set_alloc_info.emplace(re);
        }
  #endif
#endif
      } else {
        if constexpr (Raise_Exeptions)
        {
          throw std::range_error("MemPool::fmalloc: need do_OS_malloc, but it disabled");
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
      const int  deallocated  =  leaf_array[head->leaf_id].deallocated  +  real_size;
      int  available  =  leaf_array[head->leaf_id].available;
      if (deallocated  == (Leaf_Size_Bytes - available))
      {  // everything that was allocated is now returned, we will try, carefully, reset the Leaf
        leaf_array[head->leaf_id].available =  Leaf_Size_Bytes;
        leaf_array[head->leaf_id].deallocated  -=  deallocated;
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
          set_alloc_info.erase(head);
        }
  #endif
#endif
      free(head);
    }  else  {
      // this is someone else's allocation, Exception
      if constexpr (Raise_Exeptions)
      {
          throw std::range_error("MemPool::ffree: this is someone else's allocation");
      }
    }
    return;
  }

  /**
   * @brief check_access  -  checking the accessibility of the target memory area
   * @param base_alloc_ptr - the assumed address of the base allocation from MemPool
   * @param target_ptr  -  start of target memory area
   * @param target_size  -  the size of the structure to access
   * @return - true if the target memory area belongs to the base allocation from MemPool
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
    {  //  ok, this is MemPool allocation
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
              throw std::range_error("MemPool::check_access: out of allocation");
          }
        } // elseif  (start  >  target_start
      }  else {
        if constexpr (Raise_Exeptions)
        {
            throw std::range_error("MemPool::check_access: out of Leaf");
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
            throw std::range_error("MemPool::check_access: out of OS malloc allocation");
        }
      } // elseif  (start  >  target_start

    }  else  {
      if constexpr (Raise_Exeptions)
      {
          throw std::range_error("MemPool::check_access: not  MemPool's allocation");
      }
    }
    return  re;
  } // check_access

  /**
   * @brief MemPool - construct
   */
  MemPool()  noexcept
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
        leaf_array[i].available  =  Leaf_Size_Bytes;
        leaf_array[i].deallocated  =  0;
      }  else  {
        leaf_array[i].buf = nullptr;
        leaf_array[i].available  =  0;
        leaf_array[i].deallocated  =  Leaf_Size_Bytes;
      }
    }
    return;
  }  // MemPool

  /**
   * @brief instance - Singleton implementation, if anyone needs it all of a sudden
   * @return
    IMPORTANT: we rely on the linker to be able to remove duplicates of this method from all
    translation units .. provided the template parameters are the same
   */
  static MemPool * instance() {
    static MemPool  MemPool;
    return &MemPool;
  }

  ~MemPool()
  {
    for (int   i  =  0;  i  < Leaf_Cnt ;  ++i)
    {
      if (leaf_array[i].buf)  free(leaf_array[i].buf);
    }
#if defined(DEF_Auto_deallocate)
    #if defined(Debug)
    {
      for (auto &&it : map_alloc_info)
      {
        free(it.first);
      }
      map_alloc_info.clear();
    }
    #else
    {
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

  MemPool &operator=(const MemPool &) = delete;
  MemPool (const MemPool &) = delete;

  MemPool &operator=(MemPool &&) = delete;
  MemPool (MemPool &&) = delete;

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
      auto it = map_alloc_info.try_emplace(re,  AllocInfo());
      if (it.first->second.allocated)  {
        std::string err("MemPool::fmallocd: already allocated by ");
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
        auto it = map_alloc_info.find(ptr);
        if (map_alloc_info.end() == it)
        {
          throw std::range_error("MemPool::ffreed: this pointer has never been allocated");
        }
        if (!it->second.allocated)
        {
          std::string err("MemPool::ffreed: this pointer has already been freed from: ");
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
      std::string who("MemPool::check_accessd buffer overflow at ");
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
      int  available  {  Leaf_Size_Bytes  };
      // control of deallocations:
      int  deallocated  {  0  };
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
  int  cur_leaf  {  0  };

#if defined(Debug)
  struct AllocInfo {
    std::string  who;  // who performed the operation: file, code line number, in which method
    bool  allocated  {  false  };  // true - allocated, false - deallocated
  };
  std::map<void *,  AllocInfo>  map_alloc_info;


#else
  #if defined(DEF_Auto_deallocate)
  std::set<void *>  set_alloc_info;
  #endif
#endif
};


/**
   * @brief FMALLOC
   * Allocation function instead of malloc
   * @param iMemPool  -  an instance of MemPool in which we allocate
   * @param allocation_size  -  volume to allocate
   * @return - allocation ptr
*/
#if defined(Debug)
#define FMALLOC(iMemPool, allocation_size) \
   (iMemPool)->fmallocd (__FILE__, __LINE__, __FUNCTION__, allocation_size)
#else
#define FMALLOC(iMemPool, allocation_size) \
   (iMemPool)->fmalloc (allocation_size)
#endif

/**
 * @brief FFREE  -  function to release allocation instead of "free"
 * @param iMemPool  - an instance of MemPool in which we allocate
 * @param ptr  -  allocation pointer obtained earlier via fmaloc
 */
#if defined(Debug)
#define FFREE(iMemPool, ptr) \
   (iMemPool)->ffreed (__FILE__, __LINE__, __FUNCTION__, ptr)
#else
#define FFREE(iMemPool, ptr) \
   (iMemPool)->ffree (ptr)
#endif

/**
   * @brief FCHECK_ACCESS  -  checking the accessibility of the target memory area
   * @param iMemPool  - an instance of MemPool in which we allocate
   * @param base_alloc_ptr - the assumed address of the base allocation from MemPool
   * @param target_ptr  -  start of target memory area
   * @param target_size  -  the size of the structure to access
   * @return - true if the target memory area belongs to the base allocation from MemPool
   * Advantages: allows you to determine whether you have climbed into another OWN allocation,
   * while the OS swears only if it climbed out of process RAM
 */
#if defined(Debug)
#define FCHECK_ACCESS(iMemPool, base_alloc_ptr, target_ptr, target_size) \
   (iMemPool)->check_accessd (__FILE__, __LINE__, __FUNCTION__, base_alloc_ptr,  target_ptr,  target_size)
#else
#define FCHECK_ACCESS(iMemPool, base_alloc_ptr, target_ptr, target_size) \
   (iMemPool)->check_access (base_alloc_ptr,  target_ptr,  target_size)
#endif

struct MemPoolNull
{
  // Null object
};

/**
 * MemPoolAllocator
 * == std::allocator<T> template
 * Default works with SingleTone MemPool<>::instance()
 * MemPool can be injected as template method or allocation strategy:

 // inject template (Template method):
 MemPoolAllocator<std::string, MemPool<111, 11> > myAllocator;

 // inject instance (Strategy):
  using MyAllocatorType = MemPool<333, 33>;
  MyAllocatorType  MemPool;  // instance of
  MemPoolAllocator<std::string, MyAllocatorType > myAllocator(&MemPool);
 */
template<class T, class FAllocator = MemPoolNull >
struct MemAllocator : public std::allocator<T>  {
  typedef T value_type;
  using  MyAllocatorType = FAllocator;
  MyAllocatorType * p_allocator  {  nullptr  };
  MemAllocator() = default;
  MemAllocator(MyAllocatorType  *in_allocator) : p_allocator(in_allocator) { }
  template <class U> constexpr MemAllocator (const MemAllocator  <U>&)
  noexcept {}

  T* allocate(std::size_t n) {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof (T))
      throw std::bad_alloc();
    if constexpr(std::is_same<FAllocator, MemPoolNull>::value)
    {
      if (auto p = static_cast<T *>(FMALLOC(MemPool<>::instance(), (n * sizeof (T)))))
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
    if constexpr(std::is_same<FAllocator, MemPoolNull>::value)
    {
      FFREE(MemPool<>::instance(),  p);
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
bool operator==(const MemAllocator<T>&, const MemAllocator<U>&) { return true; }
template<class T, class U>
bool operator!=(const MemAllocator<T>&, const MemAllocator<U>&) { return false; }

#endif //MemPool_H
