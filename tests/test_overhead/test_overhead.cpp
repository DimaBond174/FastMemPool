#include <iostream>
#include "fast_mem_pool.h"
#include <map>
#include <functional>
#include <thread>
#include <cstdlib>
#include "specstatic.h"
#include <string>

// Importing test methods from other translation units:
extern bool test_fastmempool(int  cnt,  std::size_t each_size);
extern bool test_mempool(int  cnt,  std::size_t each_size);
extern bool test_OS_malloc(int  cnt,  std::size_t each_size);
using TestFun = std::function<bool(int  cnt,  std::size_t each_size)>;


/**
 * @brief main
 * This is the main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv)
{
  int threads_cnt = 2;
  if (argc > 1)
  {
    threads_cnt  = static_cast<int>(stoll(argv[1], static_cast<int>(strlen(argv[1]))));
    if (!threads_cnt)  threads_cnt  =  2;
  }
  //std::cout << "\nMemory overhead for each allocation bytes=" << sizeof (FastMemPool<>::AllocHeader) << std::endl;
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
  std::cout << "\nMemory overhead for each allocation bytes=" << sizeof (AllocHeader ) << std::endl;
  // For the convenience of a random choice, we will emplace these methods into a vector:

  std::map<std::string, TestFun> map_fun;

  std::cout << "\nMulti threaded (threads =" <<  threads_cnt  << "), msec for each count:";
  map_fun.emplace("|  test_fastmempool           ", test_fastmempool);
  map_fun.emplace("|  test_OS_malloc             ", test_OS_malloc);
  std::cout << "\n---------------------------------------------------------------------------------"
                << "\n|  test name, msec for allocs:|\t1000|\t10000|\t100000|\t1000000|"
                << "\n---------------------------------------------------------------------------------";

  for (auto &&it : map_fun)
  {
    std::cout << std::endl << it.first << "|\t";
    for (int cnt = 1000; cnt < 1000001; cnt *= 10)
    {
      std::vector<std::thread> vec_threads;
      int64_t start = std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
      for (int n = 0; n < threads_cnt; ++n) {
          //vec_threads.emplace_back(run_TestFun(it.second,  cnt,  256));
          vec_threads.emplace_back(it.second,  cnt,  256);
      }
      // Wait:
      for (auto& it2 : vec_threads) {
          it2.join();
      }
      int64_t end = std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
      std::cout << (end - start) << "|\t";
    }
  }
  std::cout << "\n---------------------------------------------------------------------------------";



  std::cout << "\n\nSingle threaded times, msec:";
  map_fun.emplace("|  test_mempool               ", test_mempool);

 //                          |  test name, msec for allocs:|
  std::cout << "\n---------------------------------------------------------------------------------"
                << "\n|  test name, msec for allocs:|\t1000|\t10000|\t100000|\t1000000|"
                << "\n---------------------------------------------------------------------------------";
  for (auto &&it : map_fun)
  {
    std::cout << std::endl << it.first << "|\t";
    for (int cnt = 1000; cnt < 1000001; cnt *= 10)
    {
      int64_t start = std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
      it.second(cnt, 256);
      int64_t end = std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
      std::cout << (end - start) << "|\t";
    }
  }
  std::cout << "\n---------------------------------------------------------------------------------";
  std::cout << "\nAll tests done." << std::endl;
  return 0;
}
