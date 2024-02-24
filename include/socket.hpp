#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/socket.h>

#include <coroutine>
#include <optional>
#include <span>
#include <tuple>

#include "file_descriptor.hpp"
#include "io_uring.hpp"
#include "task.hpp"

namespace couringserver {

class server_socket : public file_descriptor
{
public:
	server_socket();

	void bind(const char *port);

	void listen() const;

	class multishot_accept_guard
	{
	public:
		multishot_accept_guard(
			int raw_file_descriptor, sockaddr_storage *client_address, socklen_t *client_address_size);
		~multishot_accept_guard();

		bool await_ready() const;
		void await_suspend(std::coroutine_handle<> coroutine);
		int await_resume();

	private:
		bool initial_await_ = true;
		const int raw_file_descriptor_;
		sockaddr_storage *client_address_;
		socklen_t *client_address_size_;
		sqe_data sqe_data_;
	};

	multishot_accept_guard& accept(sockaddr_storage *client_address = nullptr, socklen_t *client_address_size = nullptr);

private:
	std::optional<multishot_accept_guard> multishot_accept_guard_;
};

class client_socket : public file_descriptor
{
public:
	explicit client_socket(int raw_file_descriptor);

	class recv_awaiter
	{
	public:
		recv_awaiter(int raw_file_descriptor, size_t length);

		bool await_ready() const;
		void await_suspend(std::coroutine_handle<> coroutine);
		std::tuple<unsigned int, ssize_t> await_resume();

	private:
		const int raw_file_descriptor_;
		const size_t length_;
		sqe_data sqe_data_;
	};

	recv_awaiter recv(size_t length);

	class send_awaiter
	{
	public:
		send_awaiter(int raw_file_descriptor, const std::span<char> &buffer, size_t length);

		bool await_ready() const;
		void await_suspend(std::coroutine_handle<> coroutine);
		ssize_t await_resume() const;

	private:
		const int raw_file_descriptor_;
		const size_t length_;
		const std::span<char> &buffer_;
		sqe_data sqe_data_;
	};

	task<ssize_t> send(const std::span<char> &buffer, size_t length);
};

} // namespace couringserver

#endif
