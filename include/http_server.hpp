#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <cstddef>
#include <thread>

#include "socket.hpp"
#include "task.hpp"
#include "thread_pool.hpp"

namespace couringserver {
class thread_worker
{
public:
	explicit thread_worker(const char *port);

	task<> accept_client();

	task<> handle_client(client_socket client_socket);

	task<> event_loop();

private:
	server_socket server_socket_;
};

class http_server
{
public:
	explicit http_server(size_t thread_count = std::thread::hardware_concurrency());

	void listen(const char *port);

private:
	thread_pool thread_pool_;
};
} // namespace couringserver

#endif
