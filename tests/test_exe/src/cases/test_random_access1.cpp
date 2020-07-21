#include "fast_mem_pool.h"
#include <iostream>
#include <vector>

extern  std::atomic_bool  keep_run;

/**
 * @brief test_random_access1
 * @return
 *  Тестируем аллокацию случайных аллокаций
 * 1) Аллоцируем случайный объём
 * 2) Берём случайную аллокацию из имеющихся и пытаемся изменить корректно
 * 3) Берём случайную аллокацию из имеющихся и пытаемся изменить заведомо с выходом за свои границы
 */
bool  test_random_access1()
{
  // Сиглтон чтобы гарантированно поконкурировать с другими нитями:
  auto && fastMemPool  =  FastMemPool<>::instance();
  struct TestStruct {
    TestStruct() :  array(nullptr), array_size (0)  {}
    TestStruct(int  *in_array,  int  in_array_size) :  array(in_array), array_size (in_array_size) {}

    int  *array;
    int  array_size;
  };
  std::vector<TestStruct>  vec_random_mem_chanks(0,  FastMemPoolAllocator<TestStruct>() ) ;
  for (int  i =  1;  i  <  1000  && keep_run.load(std::memory_order_acquire);  ++i)
  {
    int  size  =  rand() % 1000 + 4;
    void  * ptr  =  FMALLOC(fastMemPool, size);
    if (ptr) {
      vec_random_mem_chanks.emplace_back(static_cast<int  *>(ptr), (size / sizeof (int)));
    } else {
      return  false; // will throw at thread_fun()
    }
    int  id  =  rand() % i ;
    auto &&elem = vec_random_mem_chanks[id];
    if (FCHECK_ACCESS(fastMemPool, elem.array, &elem.array[elem.array_size - 1], sizeof (int))) {
      elem.array[elem.array_size - 1] = rand();
    }

    try {
      if (FCHECK_ACCESS(fastMemPool, elem.array, &elem.array[elem.array_size], sizeof (int))) {
        elem.array[elem.array_size] = rand();
        return false;
      }
    } catch (std::range_error e) {

    }
  } // for
  for (auto &&it  :  vec_random_mem_chanks) {
    FFREE(fastMemPool,  it.array);
  }
  return  true;
}
