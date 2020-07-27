#include <iostream>
#include "fast_mem_pool.h"
#include <vector>
#include <functional>
#include <thread>
#include <cstdlib>
#include "specstatic.h"

#define DEF_Raise_Exeptions false

extern bool  test_allocator1();
extern bool  test_exception1();
extern bool  test_stl_allocator2();
extern bool  test_buf_overflow1();
extern bool  test_random_access1();
extern bool  test_base_usage();

using TestFun = std::function<bool(void)>;
std::vector<TestFun> vec_fun;
std::atomic_bool  keep_run  {  true  };

void  thread_fun()
{
  int  id  = 0;
  while (keep_run.load(std::memory_order_acquire))
  {
    id  =  rand() % vec_fun.size();
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

int main(int argc, char** argv)
{
  std::allocator<int> a1;
  int  threads  =  4;
  int64_t  seconds  =  60;
  if (argc < 3)
  {
    print_usage();
  } else {
    threads  =  stoll(argv[1], strlen(argv[1]));
    if (!threads)  threads  =  2;
    seconds  =  stoll(argv[2], strlen(argv[2]));
    if (!seconds) seconds = 60;
  }

  //vec_fun.emplace_back(test_random_access1);
  //vec_fun.emplace_back(test_allocator1);
  //vec_fun.emplace_back(test_stl_allocator2);
  vec_fun.emplace_back(test_base_usage);
//  if constexpr(DEF_Raise_Exeptions)
//  {
//    vec_fun.emplace_back(test_exception1);
//    vec_fun.emplace_back(test_buf_overflow1);
//  }

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
  keep_run.store(false, std::memory_order_release);
  std::cerr << "going to stop via keep_run = false \n";
  for (auto& it : vec_threads) {
      it.join();
  }

  std::cout << "\nAll tests done." << std::endl;
  return 0;
}
