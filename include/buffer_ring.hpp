#ifndef BUFFER_RING_HPP
#define BUFFER_RING_HPP

#include <liburing/io_uring.h>

#include <bitset>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "constant.hpp"

namespace co_uring_http {

/**
 * @brief manage buffer ring
 * @details This class is a singleton class that manages the buffer ring.
 */
class buffer_ring {
public:
	static buffer_ring& get_instance() noexcept;

	// Register the buffer ring with the specified size and buffer size.
	void register_buffer_ring(const unsigned int buffer_ring_size, const size_t buffer_size);
	// Borrow a buffer from the buffer ring.
	std::span<char> borrow_buffer(const unsigned int buffer_id, const size_t size);
	// Return the buffer to the buffer ring.
	void return_buffer(const unsigned int buffer_id);

private:
	std::unique_ptr<io_uring_buf_ring> buffer_ring_;
	std::vector<std::vector<char>> buffer_list_;
	std::bitset<MAX_BUFFER_RING_SIZE> borrowed_buffer_set_;
};
} // namespace co_uring_http

#endif
