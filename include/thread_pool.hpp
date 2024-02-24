#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <list>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>

namespace couringserver {
class thread_pool
{
public:
	explicit thread_pool(std::size_t thread_count);
	~thread_pool();

	class schedule_awaiter
	{
	public:
		explicit schedule_awaiter(thread_pool &thread_pool);

		bool await_ready() const noexcept;
		void await_resume() const noexcept;
		void await_suspend(std::coroutine_handle<> handle) const noexcept;

	private:
		thread_pool &thread_pool_;
	};

	schedule_awaiter schedule();
	size_t size() const noexcept;

private:
	std::stop_source stop_source_;
	std::list<std::jthread> thread_list_;

	std::mutex mutex_;
	std::condition_variable condition_variable_;
	std::queue<std::coroutine_handle<>> coroutine_queue_;

	void thread_loop();
	void enqueue(std::coroutine_handle<> coroutine);
};
} // namespace couringserver

#endif
