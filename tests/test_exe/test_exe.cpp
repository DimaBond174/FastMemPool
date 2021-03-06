#include <iostream>
#include "fast_mem_pool.h"
#include <vector>
#include <functional>
#include <thread>
#include <cstdlib>
#include "specstatic.h"

// Importing test methods from other translation units:
extern bool  test_allocator1();
extern bool  test_exception1();
extern bool  test_stl_allocator2();
extern bool  test_random_access1();
extern bool  test_base_usage();
extern bool  test_memcontrol1();
#if defined (DEF_Auto_deallocate)
extern bool  test_auto_deallocate();
#endif

// For the convenience of a random choice, we will emplace these methods into a vector:
using TestFun = std::function<bool(void)>;
std::vector<TestFun> vec_fun;

// Global switch to terminate all tests in all threads:
std::atomic_bool  keep_run  {  true  };

/**
 * @brief thread_fun
 * Поточный запускатор методов согласно случайному выбору
 * Thread method launcher according to random selection
 */
void  thread_fun()
{
  int  id  = 0;
  while (keep_run.load(std::memory_order_acquire))
  {
    id  =  rand() % vec_fun.size();
    // Run random test:
    if (!vec_fun[id]())
    {
      throw std::range_error("Косяк! брр.. strange_error!");
    }
    std::this_thread::yield();
  }
  return;
}

void print_usage() {
  std::cout << "Usage: \n"
            << "test_exe  [threads count]  [seconds for test]\n"
            << "Default:  test_exe 4 60\n"
            << "4 threads for 60 seconds.\n";
  return;
}

/**
 * @brief main
 * This is the main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv)
{   
  FastMemPool<100000000, 1, 1024, true, true>  bulletproof_mempool;
  void *ptr = bulletproof_mempool.fmalloc(1234567);
  bulletproof_mempool.ffree(ptr);

  // How many thread_fun()'s to start:
  int  threads  =  4;
  // How long thay will test:
  int64_t  seconds  =  60;
  if (argc < 3)
  {
    print_usage();
  } else {
    threads  =  static_cast<int>(stoll(argv[1], static_cast<int>(strlen(argv[1]))));
    if (!threads)  threads  =  2;
    seconds  =  stoll(argv[2], static_cast<int>(strlen(argv[2])));
    if (!seconds) seconds = 60;
  }

  // Prepare test cases (can comment out unnecessary):
  vec_fun.emplace_back(test_memcontrol1);
  vec_fun.emplace_back(test_random_access1);
  vec_fun.emplace_back(test_allocator1);
  vec_fun.emplace_back(test_stl_allocator2);
  vec_fun.emplace_back(test_base_usage);
  if constexpr(DEF_Raise_Exeptions)
  {
    vec_fun.emplace_back(test_exception1);
  }
#if defined (DEF_Auto_deallocate)
  vec_fun.emplace_back(test_auto_deallocate);
#endif

  std::cout << "started " << threads << " threads for " << seconds << "seconds\n";

  std::vector<std::thread> vec_threads;
  for (int n = 0; n < threads; ++n) {
      vec_threads.emplace_back(thread_fun);
  }
  //std::this_thread::sleep_for(std::chrono::seconds(seconds)); //sleeps forever
  //std::this_thread::sleep_for(std::chrono::milliseconds(seconds * 1000)); //sleeps forever
  auto start = std::chrono::duration_cast<std::chrono::milliseconds>
    (std::chrono::system_clock::now().time_since_epoch()).count();
  decltype(start) msecs = seconds * 1000;
  while (std::chrono::duration_cast<std::chrono::milliseconds>
    (std::chrono::system_clock::now().time_since_epoch()).count() - start < msecs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // Stop all threads:
  keep_run.store(false, std::memory_order_release);
  std::cerr << "going to stop via keep_run = false \n";

  // Wait:
  for (auto& it : vec_threads) {
      it.join();
  }

  std::cout << "\nAll tests done." << std::endl;
  return 0;
}
