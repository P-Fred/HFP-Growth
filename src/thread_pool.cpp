#include "gimlet/thread_pool.hpp"

namespace cool {

  ThreadPool::ThreadPool(size_t nThreads) :
    threads_{}, tasks_{}, mutex_{},
    tasksToDo_{}, idle_{}, end_{false} , running_{0} {
      while(nThreads-- != 0) {      
	threads_.emplace_back([this] () {
	    std::unique_lock<std::mutex> lock(mutex_);

	    while(true) {
	      if(! tasks_.empty()) {
		function task = tasks_.front();
		tasks_.pop_front();

		++running_;
		lock.unlock();
		task();
		lock.lock();
		--running_;

		if(tasks_.empty())
		  idle_.notify_all();

	      } else if(end_) {
		break;
	      } else {
		tasksToDo_.wait(lock);
	      }
	    }
	  });
      }
    }

  ThreadPool::~ThreadPool() {
    end_ = true;
    tasksToDo_.notify_all();
    for(auto& thread : threads_) thread.join();
  }

  void ThreadPool::join() {
    std::unique_lock<std::mutex> lock(mutex_);
    while(! tasks_.empty() || running_ > 0)
      idle_.wait(lock);
  }
}
