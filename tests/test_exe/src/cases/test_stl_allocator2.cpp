#include "fast_mem_pool.h"
#include <unordered_map>
#include <string>
#include <cstdlib>

extern  std::atomic_bool  keep_run;

/**
 * @brief test_stl_allocator2
 * @return
 *  Тестируем подстановку аллокатора в STL контейнеры
 *  Testing allocator substitution in STL containers
 */
bool test_stl_allocator2()
{
  // compile time inject FastMemPoolAllocator:
  std::unordered_map<int,  int, std::hash<int>, std::equal_to<int>, FastMemPoolAllocator<std::pair<const int,  int>> >  umap1;

  // runtime inject FastMemPoolAllocator:
  std::unordered_map<int,  int>  umap2(1024, std::hash<int>(), std::equal_to<int>(),  FastMemPoolAllocator<std::pair<const int,  int>>());

  bool  sw  =  false;
  for (int  i = 0;  keep_run  &&  i < 1000000;  ++i) {
    sw = !sw;
    int  rn  =  rand();
    if (sw)
    {
      umap1.emplace(rn , rn) ;
    } else {
      umap2.emplace(rn , rn);
    }
  }

  for(auto &&it  :  umap1)
  {
    if (it.first  !=  it.second)  return false;
    if (!keep_run)  break;
  }

  for(auto &&it  :  umap2)
  {
    if (it.first  !=  it.second)  return false;
    if (!keep_run)  break;
  }

  return true;
}
