#ifndef IO_URING_HPP
#define IO_URING_HPP

#include <liburing.h>
#include <sys/socket.h>

#include <span>
#include <vector>
struct io_uring_buf_ring;
struct io_uring_cqe;

namespace co_uring_http {
struct sqe_data
{
	void *coroutine = nullptr;
	int cqe_res = 0;
	unsigned int cqe_flags = 0;
};

class io_uring
{
public:
	static io_uring &get_instance() noexcept;

	io_uring();
	~io_uring();

	io_uring(io_uring &&other) = delete;
	io_uring &operator=(io_uring &&other) = delete;
	io_uring(const io_uring &other) = delete;
	io_uring &operator=(const io_uring &other) = delete;

	class cqe_iterator
	{
	public:
		explicit cqe_iterator(const ::io_uring *io_uring, const unsigned int head);

		cqe_iterator(const cqe_iterator &) = default;

		cqe_iterator &operator++() noexcept;

		bool operator!=(const cqe_iterator &right) const noexcept;

		io_uring_cqe *operator*() const noexcept;

	private:
		const ::io_uring *io_uring_;
		unsigned int head_;
	};

	cqe_iterator begin();
	cqe_iterator end();

	void cqe_seen(io_uring_cqe *const cqe);
	int submit_and_wait(int wait_nr);

	void submit_multishot_accept_request(
		sqe_data *sqe_data, int raw_file_descriptor, sockaddr *client_addr, socklen_t *client_len);
	void submit_recv_request(sqe_data *sqe_data, int raw_file_descriptor, size_t length);
	void submit_send_request(
		sqe_data *sqe_data, int raw_file_descriptor, const std::span<char> &buffer, size_t length);
	void submit_splice_request(
		sqe_data *sqe_data, int raw_file_descriptor_in, int raw_file_descriptor_out, size_t length);
	void submit_cancel_request(sqe_data *sqe_data);

	void setup_buffer_ring(
		io_uring_buf_ring *buffer_ring, std::span<std::vector<char>> buffer_list,
		unsigned int buffer_ring_size);

	void add_buffer(
		io_uring_buf_ring *buffer_ring, std::span<char> buffer, unsigned int buffer_id,
		unsigned int buffer_ring_size);

private:
	::io_uring io_uring_;
};
} // namespace co_uring_http

#endif
