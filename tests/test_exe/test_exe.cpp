#include <iostream>
#include "fast_mem_pool.h"
#include <vector>
#include <functional>
#include <thread>
#include <cstdlib>
#include "specstatic.h"

extern bool  test_allocator1();
extern bool  test_exception1();
extern bool  test_stl_allocator2();

using TestFun = std::function<bool(void)>;
std::vector<TestFun> vec_fun;
std::atomic_bool  keep_run  {  true  };

void  thread_fun()
{
  while (keep_run)
  {
    if (!vec_fun[rand() % vec_fun.size()]())
    {
      throw std::range_error("Косяк! брр.. strange_error!");
    }
  }
  return;
}

void print_usage() {
  std::cout << "Usage: \n"
            << "test_exe  [threads count]  [seconds for test]\n"
            << "Default:  test_exe 4 60\n"
            << "4 threads for 60 seconds - now started..";
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

  vec_fun.emplace_back(test_allocator1);
  vec_fun.emplace_back(test_exception1);
  vec_fun.emplace_back(test_stl_allocator2);

  std::vector<std::thread> vec_threads;
  for (int n = 0; n < 10; ++n) {
      vec_threads.emplace_back(thread_fun);
  }
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
  keep_run = false;
  for (auto& it : vec_threads) {
      it.join();
  }

  std::cout << "\nAll tests done." << std::endl;
  return 0;
}
