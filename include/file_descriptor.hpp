#ifndef FILE_DESCRIPTOR_HPP
#define FILE_DESCRIPTOR_HPP

#include <unistd.h>

#include <compare>
#include <coroutine>
#include <filesystem>
#include <optional>
#include <tuple>

#include "io_uring.hpp"
#include "task.hpp"

namespace couringserver
{
/**
 * @brief wrapper of file descriptor
 * @details This class is a wrapper of file descriptor. It is used to manage the
 * file descriptor's life cycle. It will close the file descriptor when it is
 * destructed.
 */
class file_descriptor
{
public:
    file_descriptor();
    explicit file_descriptor(int raw_file_descriptor);
    ~file_descriptor();

    file_descriptor(file_descriptor &&other) noexcept;
    file_descriptor &operator=(file_descriptor &&other) noexcept;

    file_descriptor(const file_descriptor &other) = delete;
    file_descriptor &operator=(const file_descriptor &other) = delete;

    std::strong_ordering operator<=>(const file_descriptor &other) const;

    int get_raw_file_descriptor() const;

protected:
    std::optional<int> raw_file_descriptor_;
};

/**
 * @brief awaiter for splice operation
 * @details This class is the awaiter for the splice operation. It is used to
 * perform the splice operation asynchronously.
 */
class splice_awaiter
{
public:
    splice_awaiter(int raw_file_descriptor_in, int raw_file_descriptor_out, size_t length);

    bool await_ready() const;
    
    // This function is called when the coroutine is suspended.
    void await_suspend(std::coroutine_handle<> coroutine);
    ssize_t await_resume() const;

private:
    const int raw_file_descriptor_in_;
    const int raw_file_descriptor_out_;
    const size_t length_;
    sqe_data sqe_data_;
};

// Splice the data from one file descriptor to another.
task<ssize_t> splice(
    const file_descriptor &file_descriptor_in, const file_descriptor &file_descriptor_out,
    const size_t length);

// Create a pipe.
std::tuple<file_descriptor, file_descriptor> pipe();

// Open a file descriptor for a file.
file_descriptor open(const std::filesystem::path &path);

} // namespace couringserver

#endif
