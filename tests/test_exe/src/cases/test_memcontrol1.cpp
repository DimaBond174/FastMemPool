#include "fast_mem_pool.h"
#include "specstack.h"
#include <thread>
#include <iostream>
#include <vector>

struct VideoFrame {
  uint32_t frame_id;
  uint8_t data[256];
  VideoFrame  *nextIStack;
};

using  TVideoPool = FastMemPool<10000, 10, 1000, false, false>;
using  TWorkStack = SpecSafeStack<VideoFrame>;
extern std::atomic_bool  keep_run;

struct WorkStaff
{
  // Video frames allocator:
  TVideoPool  video_pool;
  // Video frames exchanger:
  TWorkStack  work_stack;
  std::atomic_bool  keep_detect { true };
};

// Some kind of video analytics detector
static void * people_detector(WorkStaff  *work_staff)
{
  uint32_t  idle  {  0  };
  while(work_staff->keep_detect.load(std::memory_order_acquire)
        && keep_run.load(std::memory_order_relaxed))
  {
    VideoFrame * frame = work_staff->work_stack.pop();
    if (frame)
    {
      // do detect peoples on video frame
      std::this_thread::sleep_for(std::chrono::milliseconds(30)); // slower than video_camera
      // next free frame:
      FFREE(&work_staff->video_pool, frame);
    } else {
      // no frame
      ++idle;
      std::this_thread::yield();
    }
  } // while
  return  nullptr;
}

// Video camera
static void * video_camera(WorkStaff  *work_staff)
{
  uint32_t frame_id  {  0  };
  uint32_t frames_accepted  {  0  };
  uint32_t frames_dropped  {  0  };
  while(work_staff->keep_detect.load(std::memory_order_acquire)
        && keep_run.load(std::memory_order_relaxed))
  {
    VideoFrame  * frame = static_cast<VideoFrame  *>(FMALLOC(&work_staff->video_pool, sizeof(VideoFrame)));
    if (frame)
    {
      frame->frame_id = frame_id;
      // copy real frame into  frame->data
      // next send to worker:
      work_staff->work_stack.push(frame);
      ++frames_accepted;
      //std::cout << "accepted: frame_id=" << frame_id
       //         << ", frames_accepted=" << frames_accepted << std::endl;
    } else {
      ++frames_dropped;
      //std::cout << "dropped: frame_id=" << frame_id
       //         << ", frames_dropped=" << frames_dropped << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++frame_id;
  } // while
  return  nullptr;
}

/**
 * @brief test_allocator1
 * @return
 *  Имитируем часть реально существующей системы видеоаналитики
 */
bool  test_memcontrol1()
{
  WorkStaff  work_staff;

  std::thread thread_video_camera(video_camera, &work_staff);
  std::vector<std::thread> vec_threads;
  const int  threads = 2;
  for (int n = 0; n < threads; ++n) {
      vec_threads.emplace_back(people_detector, &work_staff);
  }

  auto start = std::chrono::duration_cast<std::chrono::milliseconds>
    (std::chrono::system_clock::now().time_since_epoch()).count();
  decltype(start) msecs = 50 * 1000;
  while (std::chrono::duration_cast<std::chrono::milliseconds>
    (std::chrono::system_clock::now().time_since_epoch()).count() - start < msecs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  work_staff.keep_detect.store(false, std::memory_order_release);
  for (auto& it : vec_threads)
  {
    if (it.joinable())  it.join();
  }
  if (thread_video_camera.joinable()) thread_video_camera.join();
  //std::cout << "test_memcontrol1 ended" << std::endl;
  return true;
}
