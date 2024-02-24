#ifndef TASK_HPP
#define TASK_HPP

#include <atomic>
#include <coroutine>
#include <memory>
#include <optional>
#include <utility>

namespace co_uring_http {
template <typename T>
class task_promise;

/**
 * @brief wrapper for a coroutine
 * @details This class is a wrapper for a coroutine. It is used to manage the
 * coroutine's life cycle so that the coroutine can be scheduled and resumed 
 * in a thread-safe way.
 * @tparam T type of the return value of the coroutine
 */
template <typename T = void>
class task
{
public:
	using promise_type = task_promise<T>;

	explicit task(std::coroutine_handle<task_promise<T>> coroutine_handle) noexcept
		: coroutine_(coroutine_handle) {}

	~task() noexcept
	{
		if (coroutine_)
		{
			if (coroutine_.done())
			{
				coroutine_.destroy();
			}
			else
			{
				coroutine_.promise().get_detached_flag().test_and_set(std::memory_order_relaxed);
			}
		}
	}

	task(task &&other) noexcept : coroutine_{std::exchange(other.coroutine_, nullptr)} {}
	task &operator=(task &&other) noexcept = delete;
	task(const task &other) = delete;
	task &operator=(const task &other) = delete;

	/**
	 * @brief awaiter for task
	 * @details This class is the awaiter for the task. It is used to suspend the
	 * coroutine until the task is completed.
	 */
	class task_awaiter
	{
	public:
		explicit task_awaiter(std::coroutine_handle<task_promise<T>> coroutine)
			: coroutine_{coroutine} {}

		// task_awaiter is ready if the coroutine is done or the coroutine is null.
		bool auto await_ready() const noexcept
		{
			return coroutine_ == nullptr || coroutine_.done();
		}

		// return the result of the coroutine when the coroutine is resuming.
		decltype(auto) await_resume() const noexcept
		{
			if constexpr (!std::is_same_v<T, void>)
			{
				return coroutine_.promise().get_return_value();
			}
		}

		// suspend the calling coroutine and resume the coroutine.
		std::coroutine_handle<> await_suspend(std::coroutine_handle<> calling_coroutine) const noexcept
		{
			coroutine_.promise().get_calling_coroutine() = calling_coroutine;
			return coroutine_;
		}

	private:
		std::coroutine_handle<task_promise<T>> coroutine_;
	};

	task_awaiter operator co_await() noexcept { return task_awaiter(coroutine_); }

	// resume the coroutine.
	void resume() const noexcept
	{
		if (coroutine_ == nullptr || coroutine_.done())
		{
			return;
		}
		coroutine_.resume();
	}

	// detach the coroutine.
	void detach() noexcept
	{
		coroutine_.promise().get_detached_flag().test_and_set(std::memory_order_relaxed);
		coroutine_ = nullptr;
	}

private:
	std::coroutine_handle<task_promise<T>> coroutine_ = nullptr;
};

/**
 * @brief promise for a coroutine
 * @details This class is the promise for a coroutine. It is used to control the
 * coroutine's execution and return value.
 * @tparam T 
 */
template <typename T>
class task_promise_base
{
public:
	class final_awaiter
	{
	public:
		bool await_ready() const noexcept { return false; }

		void await_resume() const noexcept {}

		std::coroutine_handle<> await_suspend(std::coroutine_handle<task_promise<T>> coroutine) const noexcept
		{
			if (coroutine.promise().get_detached_flag().test(std::memory_order_relaxed))
			{
				coroutine.destroy();
			}
			return coroutine.promise().get_calling_coroutine().value_or(std::noop_coroutine());
		}
	};

	std::suspend_always initial_suspend() const noexcept { return {}; }

	// return to the calling coroutine when task is completed
	final_awaiter auto final_suspend() const noexcept { return final_awaiter{}; }

	void unhandled_exception() const noexcept { std::terminate(); }

	std::optional<std::coroutine_handle<>> &get_calling_coroutine() noexcept
	{
		return calling_coroutine_;
	}

	std::atomic_flag &get_detached_flag() noexcept { return detached_flag_; }

private:
	std::optional<std::coroutine_handle<>> calling_coroutine_;
	std::atomic_flag detached_flag_;
};

template <typename T>
class task_promise final : public task_promise_base<T>
{
public:
	task<T> get_return_object() noexcept
	{
		return task<T>{std::coroutine_handle<task_promise<T>>::from_promise(*this)};
	}

	template <typename U>
		requires std::convertible_to<U &&, T>
	void return_value(U &&return_value) noexcept(std::is_nothrow_constructible_v<T, U &&>)
	{
		return_value_.emplace(std::forward<U>(return_value));
	}

	T &get_return_value() & noexcept { return *return_value_; }

	T &&get_return_value() && noexcept { return std::move(*return_value_); }

private:
	std::optional<T> return_value_;
};

template <>
class task_promise<void> final : public task_promise_base<void>
{
public:
	task<void> get_return_object() noexcept
	{
		return task<void>{std::coroutine_handle<task_promise>::from_promise(*this)};
	};

	void return_void() const noexcept {}
};
} // namespace co_uring_http

#endif
