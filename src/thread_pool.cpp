#include "thread_pool.hpp"

namespace co_uring_http {
thread_pool::thread_pool(const std::size_t thread_count)
{
	for (size_t _ = 0; _ < thread_count; ++_)
	{
		thread_list_.emplace_back([&]()
								  { thread_loop(); });
	}
}

thread_pool::~thread_pool()
{
	stop_source_.request_stop();
	condition_variable_.notify_all();
}

thread_pool::schedule_awaiter::schedule_awaiter(thread_pool &thread_pool)
	: thread_pool_{thread_pool} {}

void thread_pool::schedule_awaiter::await_suspend(std::coroutine_handle<> handle) const noexcept
{
	thread_pool_.enqueue(handle);
}

bool thread_pool::schedule_awaiter::await_ready() const noexcept { return false; }

void thread_pool::schedule_awaiter::await_resume() const noexcept {}

thread_pool::schedule_awaiter thread_pool::schedule() { return schedule_awaiter{*this}; }

size_t thread_pool::size() const noexcept { return thread_list_.size(); }

void thread_pool::thread_loop()
{
	while (!stop_source_.stop_requested())
	{
		std::unique_lock lock(mutex_);
		condition_variable_.wait(lock, [this]()
								 { return stop_source_.stop_requested() || !coroutine_queue_.empty(); });
		if (stop_source_.stop_requested())
		{
			break;
		}

		const std::coroutine_handle<> coroutine = coroutine_queue_.front();
		coroutine_queue_.pop();
		lock.unlock();

		coroutine.resume();
	}
}

void thread_pool::enqueue(std::coroutine_handle<> coroutine)
{
	std::unique_lock lock(mutex_);
	coroutine_queue_.emplace(coroutine);
	condition_variable_.notify_one();
}
} // namespace co_uring_http
