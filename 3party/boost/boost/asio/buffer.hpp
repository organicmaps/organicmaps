//
// buffer.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_BUFFER_HPP
#define BOOST_ASIO_BUFFER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <string>
#include <vector>
#include <boost/detail/workaround.hpp>
#include <boost/asio/detail/array_fwd.hpp>

#if defined(BOOST_MSVC)
# if defined(_HAS_ITERATOR_DEBUGGING) && (_HAS_ITERATOR_DEBUGGING != 0)
#  if !defined(BOOST_ASIO_DISABLE_BUFFER_DEBUGGING)
#   define BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
#  endif // !defined(BOOST_ASIO_DISABLE_BUFFER_DEBUGGING)
# endif // defined(_HAS_ITERATOR_DEBUGGING)
#endif // defined(BOOST_MSVC)

#if defined(__GNUC__)
# if defined(_GLIBCXX_DEBUG)
#  if !defined(BOOST_ASIO_DISABLE_BUFFER_DEBUGGING)
#   define BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
#  endif // !defined(BOOST_ASIO_DISABLE_BUFFER_DEBUGGING)
# endif // defined(_GLIBCXX_DEBUG)
#endif // defined(__GNUC__)

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
# include <boost/function.hpp>
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x582)) \
  || BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x590))
# include <boost/type_traits/is_const.hpp>
#endif // BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x582))
       // || BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x590))

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

class mutable_buffer;
class const_buffer;

namespace detail {
void* buffer_cast_helper(const mutable_buffer&);
const void* buffer_cast_helper(const const_buffer&);
std::size_t buffer_size_helper(const mutable_buffer&);
std::size_t buffer_size_helper(const const_buffer&);
} // namespace detail

/// Holds a buffer that can be modified.
/**
 * The mutable_buffer class provides a safe representation of a buffer that can
 * be modified. It does not own the underlying data, and so is cheap to copy or
 * assign.
 */
class mutable_buffer
{
public:
  /// Construct an empty buffer.
  mutable_buffer()
    : data_(0),
      size_(0)
  {
  }

  /// Construct a buffer to represent a given memory range.
  mutable_buffer(void* data, std::size_t size)
    : data_(data),
      size_(size)
  {
  }

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
  mutable_buffer(void* data, std::size_t size,
      boost::function<void()> debug_check)
    : data_(data),
      size_(size),
      debug_check_(debug_check)
  {
  }

  const boost::function<void()>& get_debug_check() const
  {
    return debug_check_;
  }
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING

private:
  friend void* boost::asio::detail::buffer_cast_helper(
      const mutable_buffer& b);
  friend std::size_t boost::asio::detail::buffer_size_helper(
      const mutable_buffer& b);

  void* data_;
  std::size_t size_;

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
  boost::function<void()> debug_check_;
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
};

namespace detail {

inline void* buffer_cast_helper(const mutable_buffer& b)
{
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
  if (b.size_ && b.debug_check_)
    b.debug_check_();
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
  return b.data_;
}

inline std::size_t buffer_size_helper(const mutable_buffer& b)
{
  return b.size_;
}

} // namespace detail

/// Cast a non-modifiable buffer to a specified pointer to POD type.
/**
 * @relates mutable_buffer
 */
template <typename PointerToPodType>
inline PointerToPodType buffer_cast(const mutable_buffer& b)
{
  return static_cast<PointerToPodType>(detail::buffer_cast_helper(b));
}

/// Get the number of bytes in a non-modifiable buffer.
/**
 * @relates mutable_buffer
 */
inline std::size_t buffer_size(const mutable_buffer& b)
{
  return detail::buffer_size_helper(b);
}

/// Create a new modifiable buffer that is offset from the start of another.
/**
 * @relates mutable_buffer
 */
inline mutable_buffer operator+(const mutable_buffer& b, std::size_t start)
{
  if (start > buffer_size(b))
    return mutable_buffer();
  char* new_data = buffer_cast<char*>(b) + start;
  std::size_t new_size = buffer_size(b) - start;
  return mutable_buffer(new_data, new_size
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , b.get_debug_check()
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new modifiable buffer that is offset from the start of another.
/**
 * @relates mutable_buffer
 */
inline mutable_buffer operator+(std::size_t start, const mutable_buffer& b)
{
  if (start > buffer_size(b))
    return mutable_buffer();
  char* new_data = buffer_cast<char*>(b) + start;
  std::size_t new_size = buffer_size(b) - start;
  return mutable_buffer(new_data, new_size
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , b.get_debug_check()
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Adapts a single modifiable buffer so that it meets the requirements of the
/// MutableBufferSequence concept.
class mutable_buffers_1
  : public mutable_buffer
{
public:
  /// The type for each element in the list of buffers.
  typedef mutable_buffer value_type;

  /// A random-access iterator type that may be used to read elements.
  typedef const mutable_buffer* const_iterator;

  /// Construct to represent a given memory range.
  mutable_buffers_1(void* data, std::size_t size)
    : mutable_buffer(data, size)
  {
  }

  /// Construct to represent a single modifiable buffer.
  explicit mutable_buffers_1(const mutable_buffer& b)
    : mutable_buffer(b)
  {
  }

  /// Get a random-access iterator to the first element.
  const_iterator begin() const
  {
    return this;
  }

  /// Get a random-access iterator for one past the last element.
  const_iterator end() const
  {
    return begin() + 1;
  }
};

/// Holds a buffer that cannot be modified.
/**
 * The const_buffer class provides a safe representation of a buffer that cannot
 * be modified. It does not own the underlying data, and so is cheap to copy or
 * assign.
 */
class const_buffer
{
public:
  /// Construct an empty buffer.
  const_buffer()
    : data_(0),
      size_(0)
  {
  }

  /// Construct a buffer to represent a given memory range.
  const_buffer(const void* data, std::size_t size)
    : data_(data),
      size_(size)
  {
  }

  /// Construct a non-modifiable buffer from a modifiable one.
  const_buffer(const mutable_buffer& b)
    : data_(boost::asio::detail::buffer_cast_helper(b)),
      size_(boost::asio::detail::buffer_size_helper(b))
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , debug_check_(b.get_debug_check())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
  {
  }

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
  const_buffer(const void* data, std::size_t size,
      boost::function<void()> debug_check)
    : data_(data),
      size_(size),
      debug_check_(debug_check)
  {
  }

  const boost::function<void()>& get_debug_check() const
  {
    return debug_check_;
  }
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING

private:
  friend const void* boost::asio::detail::buffer_cast_helper(
      const const_buffer& b);
  friend std::size_t boost::asio::detail::buffer_size_helper(
      const const_buffer& b);

  const void* data_;
  std::size_t size_;

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
  boost::function<void()> debug_check_;
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
};

namespace detail {

inline const void* buffer_cast_helper(const const_buffer& b)
{
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
  if (b.size_ && b.debug_check_)
    b.debug_check_();
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
  return b.data_;
}

inline std::size_t buffer_size_helper(const const_buffer& b)
{
  return b.size_;
}

} // namespace detail

/// Cast a non-modifiable buffer to a specified pointer to POD type.
/**
 * @relates const_buffer
 */
template <typename PointerToPodType>
inline PointerToPodType buffer_cast(const const_buffer& b)
{
  return static_cast<PointerToPodType>(detail::buffer_cast_helper(b));
}

/// Get the number of bytes in a non-modifiable buffer.
/**
 * @relates const_buffer
 */
inline std::size_t buffer_size(const const_buffer& b)
{
  return detail::buffer_size_helper(b);
}

/// Create a new non-modifiable buffer that is offset from the start of another.
/**
 * @relates const_buffer
 */
inline const_buffer operator+(const const_buffer& b, std::size_t start)
{
  if (start > buffer_size(b))
    return const_buffer();
  const char* new_data = buffer_cast<const char*>(b) + start;
  std::size_t new_size = buffer_size(b) - start;
  return const_buffer(new_data, new_size
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , b.get_debug_check()
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new non-modifiable buffer that is offset from the start of another.
/**
 * @relates const_buffer
 */
inline const_buffer operator+(std::size_t start, const const_buffer& b)
{
  if (start > buffer_size(b))
    return const_buffer();
  const char* new_data = buffer_cast<const char*>(b) + start;
  std::size_t new_size = buffer_size(b) - start;
  return const_buffer(new_data, new_size
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , b.get_debug_check()
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Adapts a single non-modifiable buffer so that it meets the requirements of
/// the ConstBufferSequence concept.
class const_buffers_1
  : public const_buffer
{
public:
  /// The type for each element in the list of buffers.
  typedef const_buffer value_type;

  /// A random-access iterator type that may be used to read elements.
  typedef const const_buffer* const_iterator;

  /// Construct to represent a given memory range.
  const_buffers_1(const void* data, std::size_t size)
    : const_buffer(data, size)
  {
  }

  /// Construct to represent a single non-modifiable buffer.
  explicit const_buffers_1(const const_buffer& b)
    : const_buffer(b)
  {
  }

  /// Get a random-access iterator to the first element.
  const_iterator begin() const
  {
    return this;
  }

  /// Get a random-access iterator for one past the last element.
  const_iterator end() const
  {
    return begin() + 1;
  }
};

/// An implementation of both the ConstBufferSequence and MutableBufferSequence
/// concepts to represent a null buffer sequence.
class null_buffers
{
public:
  /// The type for each element in the list of buffers.
  typedef mutable_buffer value_type;

  /// A random-access iterator type that may be used to read elements.
  typedef const mutable_buffer* const_iterator;

  /// Get a random-access iterator to the first element.
  const_iterator begin() const
  {
    return &buf_;
  }

  /// Get a random-access iterator for one past the last element.
  const_iterator end() const
  {
    return &buf_;
  }

private:
  mutable_buffer buf_;
};

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
namespace detail {

template <typename Iterator>
class buffer_debug_check
{
public:
  buffer_debug_check(Iterator iter)
    : iter_(iter)
  {
  }

  ~buffer_debug_check()
  {
#if BOOST_WORKAROUND(BOOST_MSVC, == 1400)
    // MSVC 8's string iterator checking may crash in a std::string::iterator
    // object's destructor when the iterator points to an already-destroyed
    // std::string object, unless the iterator is cleared first.
    iter_ = Iterator();
#endif // BOOST_WORKAROUND(BOOST_MSVC, == 1400)
  }

  void operator()()
  {
    *iter_;
  }

private:
  Iterator iter_;
};

} // namespace detail
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING

/** @defgroup buffer boost::asio::buffer
 *
 * @brief The boost::asio::buffer function is used to create a buffer object to
 * represent raw memory, an array of POD elements, a vector of POD elements,
 * or a std::string.
 *
 * A buffer object represents a contiguous region of memory as a 2-tuple
 * consisting of a pointer and size in bytes. A tuple of the form <tt>{void*,
 * size_t}</tt> specifies a mutable (modifiable) region of memory. Similarly, a
 * tuple of the form <tt>{const void*, size_t}</tt> specifies a const
 * (non-modifiable) region of memory. These two forms correspond to the classes
 * mutable_buffer and const_buffer, respectively. To mirror C++'s conversion
 * rules, a mutable_buffer is implicitly convertible to a const_buffer, and the
 * opposite conversion is not permitted.
 *
 * The simplest use case involves reading or writing a single buffer of a
 * specified size:
 *
 * @code sock.send(boost::asio::buffer(data, size)); @endcode
 *
 * In the above example, the return value of boost::asio::buffer meets the
 * requirements of the ConstBufferSequence concept so that it may be directly
 * passed to the socket's write function. A buffer created for modifiable
 * memory also meets the requirements of the MutableBufferSequence concept.
 *
 * An individual buffer may be created from a builtin array, std::vector or
 * boost::array of POD elements. This helps prevent buffer overruns by
 * automatically determining the size of the buffer:
 *
 * @code char d1[128];
 * size_t bytes_transferred = sock.receive(boost::asio::buffer(d1));
 *
 * std::vector<char> d2(128);
 * bytes_transferred = sock.receive(boost::asio::buffer(d2));
 *
 * boost::array<char, 128> d3;
 * bytes_transferred = sock.receive(boost::asio::buffer(d3)); @endcode
 *
 * In all three cases above, the buffers created are exactly 128 bytes long.
 * Note that a vector is @e never automatically resized when creating or using
 * a buffer. The buffer size is determined using the vector's <tt>size()</tt>
 * member function, and not its capacity.
 *
 * @par Accessing Buffer Contents
 *
 * The contents of a buffer may be accessed using the boost::asio::buffer_size
 * and boost::asio::buffer_cast functions:
 *
 * @code boost::asio::mutable_buffer b1 = ...;
 * std::size_t s1 = boost::asio::buffer_size(b1);
 * unsigned char* p1 = boost::asio::buffer_cast<unsigned char*>(b1);
 *
 * boost::asio::const_buffer b2 = ...;
 * std::size_t s2 = boost::asio::buffer_size(b2);
 * const void* p2 = boost::asio::buffer_cast<const void*>(b2); @endcode
 *
 * The boost::asio::buffer_cast function permits violations of type safety, so
 * uses of it in application code should be carefully considered.
 *
 * @par Buffer Invalidation
 *
 * A buffer object does not have any ownership of the memory it refers to. It
 * is the responsibility of the application to ensure the memory region remains
 * valid until it is no longer required for an I/O operation. When the memory
 * is no longer available, the buffer is said to have been invalidated.
 *
 * For the boost::asio::buffer overloads that accept an argument of type
 * std::vector, the buffer objects returned are invalidated by any vector
 * operation that also invalidates all references, pointers and iterators
 * referring to the elements in the sequence (C++ Std, 23.2.4)
 *
 * For the boost::asio::buffer overloads that accept an argument of type
 * std::string, the buffer objects returned are invalidated according to the
 * rules defined for invalidation of references, pointers and iterators
 * referring to elements of the sequence (C++ Std, 21.3).
 *
 * @par Buffer Arithmetic
 *
 * Buffer objects may be manipulated using simple arithmetic in a safe way
 * which helps prevent buffer overruns. Consider an array initialised as
 * follows:
 *
 * @code boost::array<char, 6> a = { 'a', 'b', 'c', 'd', 'e' }; @endcode
 *
 * A buffer object @c b1 created using:
 *
 * @code b1 = boost::asio::buffer(a); @endcode
 *
 * represents the entire array, <tt>{ 'a', 'b', 'c', 'd', 'e' }</tt>. An
 * optional second argument to the boost::asio::buffer function may be used to
 * limit the size, in bytes, of the buffer:
 *
 * @code b2 = boost::asio::buffer(a, 3); @endcode
 *
 * such that @c b2 represents the data <tt>{ 'a', 'b', 'c' }</tt>. Even if the
 * size argument exceeds the actual size of the array, the size of the buffer
 * object created will be limited to the array size.
 *
 * An offset may be applied to an existing buffer to create a new one:
 *
 * @code b3 = b1 + 2; @endcode
 *
 * where @c b3 will set to represent <tt>{ 'c', 'd', 'e' }</tt>. If the offset
 * exceeds the size of the existing buffer, the newly created buffer will be
 * empty.
 *
 * Both an offset and size may be specified to create a buffer that corresponds
 * to a specific range of bytes within an existing buffer:
 *
 * @code b4 = boost::asio::buffer(b1 + 1, 3); @endcode
 *
 * so that @c b4 will refer to the bytes <tt>{ 'b', 'c', 'd' }</tt>.
 *
 * @par Buffers and Scatter-Gather I/O
 *
 * To read or write using multiple buffers (i.e. scatter-gather I/O), multiple
 * buffer objects may be assigned into a container that supports the
 * MutableBufferSequence (for read) or ConstBufferSequence (for write) concepts:
 *
 * @code
 * char d1[128];
 * std::vector<char> d2(128);
 * boost::array<char, 128> d3;
 *
 * boost::array<mutable_buffer, 3> bufs1 = {
 *   boost::asio::buffer(d1),
 *   boost::asio::buffer(d2),
 *   boost::asio::buffer(d3) };
 * bytes_transferred = sock.receive(bufs1);
 *
 * std::vector<const_buffer> bufs2;
 * bufs2.push_back(boost::asio::buffer(d1));
 * bufs2.push_back(boost::asio::buffer(d2));
 * bufs2.push_back(boost::asio::buffer(d3));
 * bytes_transferred = sock.send(bufs2); @endcode
 */
/*@{*/

/// Create a new modifiable buffer from an existing buffer.
/**
 * @returns <tt>mutable_buffers_1(b)</tt>.
 */
inline mutable_buffers_1 buffer(const mutable_buffer& b)
{
  return mutable_buffers_1(b);
}

/// Create a new modifiable buffer from an existing buffer.
/**
 * @returns A mutable_buffers_1 value equivalent to:
 * @code mutable_buffers_1(
 *     buffer_cast<void*>(b),
 *     min(buffer_size(b), max_size_in_bytes)); @endcode
 */
inline mutable_buffers_1 buffer(const mutable_buffer& b,
    std::size_t max_size_in_bytes)
{
  return mutable_buffers_1(
      mutable_buffer(buffer_cast<void*>(b),
        buffer_size(b) < max_size_in_bytes
        ? buffer_size(b) : max_size_in_bytes
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
        , b.get_debug_check()
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
        ));
}

/// Create a new non-modifiable buffer from an existing buffer.
/**
 * @returns <tt>const_buffers_1(b)</tt>.
 */
inline const_buffers_1 buffer(const const_buffer& b)
{
  return const_buffers_1(b);
}

/// Create a new non-modifiable buffer from an existing buffer.
/**
 * @returns A const_buffers_1 value equivalent to:
 * @code const_buffers_1(
 *     buffer_cast<const void*>(b),
 *     min(buffer_size(b), max_size_in_bytes)); @endcode
 */
inline const_buffers_1 buffer(const const_buffer& b,
    std::size_t max_size_in_bytes)
{
  return const_buffers_1(
      const_buffer(buffer_cast<const void*>(b),
        buffer_size(b) < max_size_in_bytes
        ? buffer_size(b) : max_size_in_bytes
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
        , b.get_debug_check()
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
        ));
}

/// Create a new modifiable buffer that represents the given memory range.
/**
 * @returns <tt>mutable_buffers_1(data, size_in_bytes)</tt>.
 */
inline mutable_buffers_1 buffer(void* data, std::size_t size_in_bytes)
{
  return mutable_buffers_1(mutable_buffer(data, size_in_bytes));
}

/// Create a new non-modifiable buffer that represents the given memory range.
/**
 * @returns <tt>const_buffers_1(data, size_in_bytes)</tt>.
 */
inline const_buffers_1 buffer(const void* data,
    std::size_t size_in_bytes)
{
  return const_buffers_1(const_buffer(data, size_in_bytes));
}

/// Create a new modifiable buffer that represents the given POD array.
/**
 * @returns A mutable_buffers_1 value equivalent to:
 * @code mutable_buffers_1(
 *     static_cast<void*>(data),
 *     N * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline mutable_buffers_1 buffer(PodType (&data)[N])
{
  return mutable_buffers_1(mutable_buffer(data, N * sizeof(PodType)));
}
 
/// Create a new modifiable buffer that represents the given POD array.
/**
 * @returns A mutable_buffers_1 value equivalent to:
 * @code mutable_buffers_1(
 *     static_cast<void*>(data),
 *     min(N * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline mutable_buffers_1 buffer(PodType (&data)[N],
    std::size_t max_size_in_bytes)
{
  return mutable_buffers_1(
      mutable_buffer(data,
        N * sizeof(PodType) < max_size_in_bytes
        ? N * sizeof(PodType) : max_size_in_bytes));
}
 
/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffers_1 value equivalent to:
 * @code const_buffers_1(
 *     static_cast<const void*>(data),
 *     N * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline const_buffers_1 buffer(const PodType (&data)[N])
{
  return const_buffers_1(const_buffer(data, N * sizeof(PodType)));
}

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffers_1 value equivalent to:
 * @code const_buffers_1(
 *     static_cast<const void*>(data),
 *     min(N * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline const_buffers_1 buffer(const PodType (&data)[N],
    std::size_t max_size_in_bytes)
{
  return const_buffers_1(
      const_buffer(data,
        N * sizeof(PodType) < max_size_in_bytes
        ? N * sizeof(PodType) : max_size_in_bytes));
}

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x582)) \
  || BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x590))

// Borland C++ and Sun Studio think the overloads:
//
//   unspecified buffer(boost::array<PodType, N>& array ...);
//
// and
//
//   unspecified buffer(boost::array<const PodType, N>& array ...);
//
// are ambiguous. This will be worked around by using a buffer_types traits
// class that contains typedefs for the appropriate buffer and container
// classes, based on whether PodType is const or non-const.

namespace detail {

template <bool IsConst>
struct buffer_types_base;

template <>
struct buffer_types_base<false>
{
  typedef mutable_buffer buffer_type;
  typedef mutable_buffers_1 container_type;
};

template <>
struct buffer_types_base<true>
{
  typedef const_buffer buffer_type;
  typedef const_buffers_1 container_type;
};

template <typename PodType>
struct buffer_types
  : public buffer_types_base<boost::is_const<PodType>::value>
{
};

} // namespace detail

template <typename PodType, std::size_t N>
inline typename detail::buffer_types<PodType>::container_type
buffer(boost::array<PodType, N>& data)
{
  typedef typename boost::asio::detail::buffer_types<PodType>::buffer_type
    buffer_type;
  typedef typename boost::asio::detail::buffer_types<PodType>::container_type
    container_type;
  return container_type(
      buffer_type(data.c_array(), data.size() * sizeof(PodType)));
}

template <typename PodType, std::size_t N>
inline typename detail::buffer_types<PodType>::container_type
buffer(boost::array<PodType, N>& data, std::size_t max_size_in_bytes)
{
  typedef typename boost::asio::detail::buffer_types<PodType>::buffer_type
    buffer_type;
  typedef typename boost::asio::detail::buffer_types<PodType>::container_type
    container_type;
  return container_type(
      buffer_type(data.c_array(),
        data.size() * sizeof(PodType) < max_size_in_bytes
        ? data.size() * sizeof(PodType) : max_size_in_bytes));
}

#else // BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x582))
      // || BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x590))

/// Create a new modifiable buffer that represents the given POD array.
/**
 * @returns A mutable_buffers_1 value equivalent to:
 * @code mutable_buffers_1(
 *     data.data(),
 *     data.size() * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline mutable_buffers_1 buffer(boost::array<PodType, N>& data)
{
  return mutable_buffers_1(
      mutable_buffer(data.c_array(), data.size() * sizeof(PodType)));
}

/// Create a new modifiable buffer that represents the given POD array.
/**
 * @returns A mutable_buffers_1 value equivalent to:
 * @code mutable_buffers_1(
 *     data.data(),
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline mutable_buffers_1 buffer(boost::array<PodType, N>& data,
    std::size_t max_size_in_bytes)
{
  return mutable_buffers_1(
      mutable_buffer(data.c_array(),
        data.size() * sizeof(PodType) < max_size_in_bytes
        ? data.size() * sizeof(PodType) : max_size_in_bytes));
}

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffers_1 value equivalent to:
 * @code const_buffers_1(
 *     data.data(),
 *     data.size() * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline const_buffers_1 buffer(boost::array<const PodType, N>& data)
{
  return const_buffers_1(
      const_buffer(data.data(), data.size() * sizeof(PodType)));
}

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffers_1 value equivalent to:
 * @code const_buffers_1(
 *     data.data(),
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline const_buffers_1 buffer(boost::array<const PodType, N>& data,
    std::size_t max_size_in_bytes)
{
  return const_buffers_1(
      const_buffer(data.data(),
        data.size() * sizeof(PodType) < max_size_in_bytes
        ? data.size() * sizeof(PodType) : max_size_in_bytes));
}

#endif // BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x582))
       // || BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x590))

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffers_1 value equivalent to:
 * @code const_buffers_1(
 *     data.data(),
 *     data.size() * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline const_buffers_1 buffer(const boost::array<PodType, N>& data)
{
  return const_buffers_1(
      const_buffer(data.data(), data.size() * sizeof(PodType)));
}

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffers_1 value equivalent to:
 * @code const_buffers_1(
 *     data.data(),
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline const_buffers_1 buffer(const boost::array<PodType, N>& data,
    std::size_t max_size_in_bytes)
{
  return const_buffers_1(
      const_buffer(data.data(),
        data.size() * sizeof(PodType) < max_size_in_bytes
        ? data.size() * sizeof(PodType) : max_size_in_bytes));
}

/// Create a new modifiable buffer that represents the given POD vector.
/**
 * @returns A mutable_buffers_1 value equivalent to:
 * @code mutable_buffers_1(
 *     data.size() ? &data[0] : 0,
 *     data.size() * sizeof(PodType)); @endcode
 *
 * @note The buffer is invalidated by any vector operation that would also
 * invalidate iterators.
 */
template <typename PodType, typename Allocator>
inline mutable_buffers_1 buffer(std::vector<PodType, Allocator>& data)
{
  return mutable_buffers_1(
      mutable_buffer(data.size() ? &data[0] : 0, data.size() * sizeof(PodType)
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
        , detail::buffer_debug_check<
            typename std::vector<PodType, Allocator>::iterator
          >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
        ));
}

/// Create a new modifiable buffer that represents the given POD vector.
/**
 * @returns A mutable_buffers_1 value equivalent to:
 * @code mutable_buffers_1(
 *     data.size() ? &data[0] : 0,
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 *
 * @note The buffer is invalidated by any vector operation that would also
 * invalidate iterators.
 */
template <typename PodType, typename Allocator>
inline mutable_buffers_1 buffer(std::vector<PodType, Allocator>& data,
    std::size_t max_size_in_bytes)
{
  return mutable_buffers_1(
      mutable_buffer(data.size() ? &data[0] : 0,
        data.size() * sizeof(PodType) < max_size_in_bytes
        ? data.size() * sizeof(PodType) : max_size_in_bytes
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
        , detail::buffer_debug_check<
            typename std::vector<PodType, Allocator>::iterator
          >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
        ));
}

/// Create a new non-modifiable buffer that represents the given POD vector.
/**
 * @returns A const_buffers_1 value equivalent to:
 * @code const_buffers_1(
 *     data.size() ? &data[0] : 0,
 *     data.size() * sizeof(PodType)); @endcode
 *
 * @note The buffer is invalidated by any vector operation that would also
 * invalidate iterators.
 */
template <typename PodType, typename Allocator>
inline const_buffers_1 buffer(
    const std::vector<PodType, Allocator>& data)
{
  return const_buffers_1(
      const_buffer(data.size() ? &data[0] : 0, data.size() * sizeof(PodType)
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
        , detail::buffer_debug_check<
            typename std::vector<PodType, Allocator>::const_iterator
          >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
        ));
}

/// Create a new non-modifiable buffer that represents the given POD vector.
/**
 * @returns A const_buffers_1 value equivalent to:
 * @code const_buffers_1(
 *     data.size() ? &data[0] : 0,
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 *
 * @note The buffer is invalidated by any vector operation that would also
 * invalidate iterators.
 */
template <typename PodType, typename Allocator>
inline const_buffers_1 buffer(
    const std::vector<PodType, Allocator>& data, std::size_t max_size_in_bytes)
{
  return const_buffers_1(
      const_buffer(data.size() ? &data[0] : 0,
        data.size() * sizeof(PodType) < max_size_in_bytes
        ? data.size() * sizeof(PodType) : max_size_in_bytes
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
        , detail::buffer_debug_check<
            typename std::vector<PodType, Allocator>::const_iterator
          >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
        ));
}

/// Create a new non-modifiable buffer that represents the given string.
/**
 * @returns <tt>const_buffers_1(data.data(), data.size())</tt>.
 *
 * @note The buffer is invalidated by any non-const operation called on the
 * given string object.
 */
inline const_buffers_1 buffer(const std::string& data)
{
  return const_buffers_1(const_buffer(data.data(), data.size()
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
        , detail::buffer_debug_check<std::string::const_iterator>(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
        ));
}

/// Create a new non-modifiable buffer that represents the given string.
/**
 * @returns A const_buffers_1 value equivalent to:
 * @code const_buffers_1(
 *     data.data(),
 *     min(data.size(), max_size_in_bytes)); @endcode
 *
 * @note The buffer is invalidated by any non-const operation called on the
 * given string object.
 */
inline const_buffers_1 buffer(const std::string& data,
    std::size_t max_size_in_bytes)
{
  return const_buffers_1(
      const_buffer(data.data(),
        data.size() < max_size_in_bytes
        ? data.size() : max_size_in_bytes
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
        , detail::buffer_debug_check<std::string::const_iterator>(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
        ));
}

/*@}*/

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_BUFFER_HPP
