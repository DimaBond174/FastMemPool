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
 * Вашему вниманию простой template аллокатора с потокобезопасным циклическим буфером.
 * Судя по обилию новых моделей велосипедов на современных улицах,
 * понятие "не надо изобретать велосипед" больше не актуально..
 *
 * Преимущества:
 * 1) Скорость работы
 * 2) Потокобезопасный Аллокатор без мьютексов или циклов ожидания
 * 3) Функционал защиты доступа к памяти: опознаёт свои аллокации,
 * а также контроллирует выход в чужую область, в том числе между своими аллокациями.
 * ( в то время как OS жалуется только если залезли в чужую память )
 * 4) Быстрая деаллокация ( нет затрат на дополнительные структуры реестров аллокаций )
 * 5) Если закончился свой пул RAM, аллокация будет произведена через OS malloc, но при этом будет
 * сохранен функционал 2) защиты доступа к памяти в полном объёме.
 * Дополнительно:
 *  - возможно ведения реестра аллокаций для отладки ( включается в код в compile time если defined(Debug))
 *  Например: при повторной деаллокации, в Exception раскажет где произошла первая:
 * "FastMemPool::ffreed: this pointer has already been freed from: test_exe.cpp, at 9  line, in free1"
 *
 * Налагаемые обязательства:
 *  - для работы циклического буфера FastMemPool будет
 *    аллоцировано Leaf_Cnt листов памяти размером Leaf_Size_Bytes.
 *    Если Leaf_Size_Bytes не хватит для требуемой аллокации, то будет выполнен OS malloc -
 *    поэтому Leaf_Size_Bytes должен быть значительно больше среднестатистической Вашей аллокации.
 *
 * - желательно (но не обязательно) чтобы аллокации проходили цикл равномерно,
 *   Например: если аллокации идут для сессий пользователей, то эти сессии должны быть ограничены во времени.
 *  (листы аллокаций возвращаются в работу после истощения с последней деаллокацией на своём листе,
 *   если кто-то держит аллокацию бесконечно, то соответствующий один из Leaf_Cnt листов будет простаивать.
 *   если заблокировать все Leaf_Cnt листов, то преимуществом перед OS malloc останется только контроль памяти)
 *
 * - желательно FastMemPool конструировать когда ещё на сервере оперативной памяти в избытке, так как
 *   если из Leaf_Cnt листов несколько OS не сможет аллоцировать, то эти nullptr прорешины не будут работать.
 *
 * Примеры использования:
 * Обычное, с  определением размера каждого из Leaf_Cnt листов памяти:
 *
 * Подключение ведения списка аллокаций  для контроля и автодеаллокации:
 *
 * Подключение вызова Exeptions  на ошибки:
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
   * Функция аллокации вместо malloc
   * @param allocation_size  -  аллоцируемый объём
   * @return
   */
  void  * fmalloc(std::size_t  allocation_size)  noexcept
  {
    // Аллокация будет в себя включать заголовок со служебной информацией
    const int  real_size = allocation_size  +  sizeof(AllocHeader);
    // Текущий лист RAM который сейчас распределяется, но в целом всё равно откуда:
    const int start_leaf = cur_leaf.load(std::memory_order_relaxed);
    // Идентификатор листа для циклического обхода:
    int leaf_id = start_leaf;
    // Результирующая аллокация:
    char  *re  =  nullptr;

    /*
     * Цикл поиска доступной аллокации пройдёт по кругу leaf_array за счёт переполнения uint8_t.
     * Выход из цикла в момент завершения круга прохода когда  снова встретим start_leaf.
     * В случае невозможности сделать аллокацию в собственном пуле памяти,
     *  произойдёт эскалация до OS malloc, при этом функционал по контролю доступа останется рабочим.
   */
    do {
      const int available  =  leaf_array[leaf_id].available.load(std::memory_order_acquire);
      if (available  >=  real_size)
      {
        // резервируем операцию  = защита от сброса Leaf в исходное:
        //leaf_array[leaf_id].allocated.fetch_add(real_size, std::memory_order_release);
        // резервируем память (раздача буфера идёт с конца откусыванем):
        const int  available_after  =  leaf_array[leaf_id].available.fetch_sub(real_size, std::memory_order_acq_rel)  -  real_size;
        // и в случае успеха остаться должно было вернуться положительное число,
        // иначе мы бы пробили дно буфера:
        if (available_after >= 0)
        {  // результирующий адрес аллокации легко получить, ведь он начинается
          // сразу за available, т.к. адресация с [0] то это и есть available:
          re  =  leaf_array[leaf_id].buf + available_after;
          if (available_after < Average_Allocation)
          {  // Расскажем остальным потокам что следует использовать другую страницу памяти:
            const int next_id = start_leaf + 1;
            if (next_id >= Leaf_Cnt)
            {
              cur_leaf.store(0, std::memory_order_release);
            } else {
              cur_leaf.store(next_id, std::memory_order_release);
            }
          }
          break; // закончили поиск, отдаём указатель аллокации
        }
      }
      ++leaf_id;
      if (Leaf_Cnt == leaf_id)  {  leaf_id  =  0;  }
    } while (leaf_id  !=  start_leaf);

    bool  do_OS_malloc  =  !re;
    if  (do_OS_malloc)
    {  // Вот сейчас произойдёт эскалация до OS malloc:
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
    {
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
   * @brief ffree  -  функция освобождения аллокации вместо free
   * @param ptr  -  указатель на аллокацию через fmaloc
   */
  void  ffree(void  *ptr)
  {
    char  *to_free  =  static_cast<char  *>(ptr)  -  sizeof(AllocHeader);
    AllocHeader  *head  =  reinterpret_cast<AllocHeader  *>(to_free);
    if  (0 <= head->size  &&  head->size < Leaf_Size_Bytes
         &&  0 <= head->leaf_id  &&  head->leaf_id < Leaf_Cnt
         && ((uint64_t)this) == (head->tag_this - head->leaf_id)
         &&  leaf_array[head->leaf_id].buf)
    {  //  ок, это моя аллокация
      const int  real_size = head->size  +  sizeof(AllocHeader);
      const int  deallocated  =  leaf_array[head->leaf_id].deallocated.fetch_add(real_size, std::memory_order_acq_rel)  +  real_size;
      int  available  =  leaf_array[head->leaf_id].available.load(std::memory_order_acquire);
      if (deallocated  == (Leaf_Size_Bytes - available))
      {  // всё что было аллоцировано теперь возвращено, попытаемся, осторожно, сбросить Leaf
        if (leaf_array[head->leaf_id].available.compare_exchange_strong(available,  Leaf_Size_Bytes))
        {
          leaf_array[head->leaf_id].deallocated  -=  deallocated;
        }
      }

      // Зачистка чтобы уникальные TAG_my_alloc  не перестали быть уникальными:
      memset(head,  0,  sizeof(AllocHeader));
    }  else if (TAG_OS_malloc  ==  head->tag_this
         &&  OS_malloc_id  ==  head->leaf_id
         &&  head->size > 0   )
    {  // ok, это OS malloc
      // Зачистка чтобы уникальные TAG_my_alloc  не перестали быть уникальными:
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
      // это чужая аллокация, Exception
      if constexpr (Raise_Exeptions)
      {
          throw std::range_error("FastMemPool::ffree: this is someone else's allocation");
      }
    }
    return;
  }

  /**
   * @brief check_access  -  проверка возможности доступа к целевой области памяти
   * @param base_alloc_ptr - предполагаемый адрес базовой аллокации из FastMemPool
   * @param target_ptr  -  начало целевой области памяти
   * @param target_size  -  размер структуры, к которой нужен доступ
   * @return - true если целевая область памяти принадлежит базовой аллокации из FastMemPool
   * Преимущества: позволяет определить не залез ли в другую СВОЮ аллокацию,
   * в то время как OS ругается только если залез в чужую.
   */
  bool  check_access(void  *base_alloc_ptr,  void  *target_ptr,  std::size_t  target_size)
  {
    bool  re  = false;
    AllocHeader  *head  =  reinterpret_cast<AllocHeader  *>(static_cast<char  *>(base_alloc_ptr)  -  sizeof(AllocHeader));
    if  (0 <= head->size  &&  head->size < Leaf_Size_Bytes
         &&  0 <= head->leaf_id  &&  head->leaf_id < Leaf_Cnt
         && ((uint64_t)this) == (head->tag_this - head->leaf_id))
    {  //  ок, это FastMemPool аллокация
      // проверим этого ли экземпляра FastMemPool:
      char  *start  =  static_cast<char  *>(base_alloc_ptr);
      char  *end  =  start  +  head->size;
      char  *buf  =  leaf_array[head->leaf_id].buf;
      if (buf  &&  buf  <=  start  &&  (buf  +  Leaf_Size_Bytes) >= end)
      { // Проверим вышел ли за пределы аллокации:
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
      // Проверим вышел ли за пределы аллокации:
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
   * @brief FastMemPool - конструктор
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
   * @brief instance - Реализация синглтона, если кому необходим вдруг
   * @return
   * ВАЖНО: полагаемся на то что линковщик сможет убрать дубли этого метода
   * из всех translation unit -ов.. при условии одинаковых параметров шаблона
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
   * Декоратор для метода fmalloc - сохраняет информацию о месте аллокации
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
   * Декоратор для метода ffree
   * - сохраняет информацию о месте деаллокации
   * - в случе повторной деаллокации рассказывает где произошла первая деаллокация
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
      // available == offset наоборот
      std::atomic<int>  available  {  Leaf_Size_Bytes  };
      // allocated == контроль деаллокаций
      std::atomic<int>  deallocated  {  0  };
  };

  /*
    Заголовок каждой аллокации
    Позволяет быстро оценить своя ли аллокация и откуда:
    tag_this - уникальное число, маловероятное к встрече в RAM,
      при этом идёт взаимная проверка с leaf_id:
      если tag - leaf_id  !=  this  или для OS malloc != -OS_malloc_id,  значит это чужая аллокация
    leaf_id - если отрицательное {  -2020071708  } => аллокация была сделана в OS malloc,
          если положительное { 0 .. 255 } => аллокация в leaf_array[i],
          иначе это не наша аллокация и мы с ней  не работаем
     size - размер аллокации, если 0 <=  size >=  LeafSizeBytes  - значит чужая аллокация.
     Заголовок можно получить в любой момент отрицательным смещением относительно адреса
     аллокации про который знает хозяин аллокации.
*/
  const int  OS_malloc_id  {  -2020071708  };
  const int  TAG_OS_malloc  {  1020071708  };
  struct AllocHeader {
    uint64_t  tag_this  {  2020071700  };  //  метка своих аллокаций: this + leaf_id
    int  size;  // размер аллокации
    int  leaf_id  {  -2020071708  };  // быстрый доступ к листу аллокации
  };

  Leaf  leaf_array[Leaf_Cnt];
  std::atomic<int>  cur_leaf  {  0  };
#if defined(Debug)
  struct AllocInfo {
    std::string  who;  // кто произвёл операцию: файл, номер строки кода, в каком методе
    bool  allocated  {  false  };  //  true - аллоцировано,  false - деаллоцировано
  };
  std::map<void *,  AllocInfo>  map_alloc_info;
  std::mutex  mut_map_alloc_info;
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
   * Функция аллокации вместо malloc
   * @param iFastMemPool  -  экземпляр FastMemPool в котором проводим аллокацию
   * @param allocation_size  -  аллоцируемый объём
   * @return
*/
#if defined(Debug)
#define FMALLOC(iFastMemPool, allocation_size) \
   (iFastMemPool)->fmallocd (__FILE__, __LINE__, __FUNCTION__, allocation_size)
#else
#define FMALLOC(iFastMemPool, allocation_size) \
   (iFastMemPool)->fmalloc (allocation_size)
#endif

/**
 * @brief FFREE  -  функция освобождения аллокации вместо free
 * @param iFastMemPool  -  экземпляр FastMemPool в котором проводили аллокацию
 * @param ptr  -  указатель на аллокацию через fmaloc
 */
#if defined(Debug)
#define FFREE(iFastMemPool, ptr) \
   (iFastMemPool)->ffreed (__FILE__, __LINE__, __FUNCTION__, ptr)
#else
#define FFREE(iFastMemPool, ptr) \
   (iFastMemPool)->ffree (ptr)
#endif

/**
   * @brief FCHECK_ACCESS  -  проверка возможности доступа к целевой области памяти
   * @param base_alloc_ptr - предполагаемый адрес базовой аллокации из FastMemPool
   * @param target_ptr  -  начало целевой области памяти
   * @param target_size  -  размер структуры, к которой нужен доступ
   * @return - true если целевая область памяти принадлежит базовой аллокации из FastMemPool
   * Преимущества: позволяет определить не залез ли в другую СВОЮ аллокацию,
   * в то время как OS ругается только если залез в чужую.
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
