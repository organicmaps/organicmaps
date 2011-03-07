//
// detail/config.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_CONFIG_HPP
#define BOOST_ASIO_DETAIL_CONFIG_HPP

#include <boost/config.hpp>

// Default to a header-only implementation. The user must specifically request
// separate compilation by defining either BOOST_ASIO_SEPARATE_COMPILATION or
// BOOST_ASIO_DYN_LINK (as a DLL/shared library implies separate compilation).
#if !defined(BOOST_ASIO_HEADER_ONLY)
# if !defined(BOOST_ASIO_SEPARATE_COMPILATION)
#  if !defined(BOOST_ASIO_DYN_LINK)
#   define BOOST_ASIO_HEADER_ONLY
#  endif // !defined(BOOST_ASIO_DYN_LINK)
# endif // !defined(BOOST_ASIO_SEPARATE_COMPILATION)
#endif // !defined(BOOST_ASIO_HEADER_ONLY)

#if defined(BOOST_ASIO_HEADER_ONLY)
# define BOOST_ASIO_DECL inline
#else // defined(BOOST_ASIO_HEADER_ONLY)
# if defined(BOOST_HAS_DECLSPEC)
// We need to import/export our code only if the user has specifically asked
// for it by defining BOOST_ASIO_DYN_LINK.
#  if defined(BOOST_ASIO_DYN_LINK)
// Export if this is our own source, otherwise import.
#   if defined(BOOST_ASIO_SOURCE)
#    define BOOST_ASIO_DECL __declspec(dllexport)
#   else // defined(BOOST_ASIO_SOURCE)
#    define BOOST_ASIO_DECL __declspec(dllimport)
#   endif // defined(BOOST_ASIO_SOURCE)
#  endif // defined(BOOST_ASIO_DYN_LINK)
# endif // defined(BOOST_HAS_DECLSPEC)
#endif // defined(BOOST_ASIO_HEADER_ONLY)

// If BOOST_ASIO_DECL isn't defined yet define it now.
#if !defined(BOOST_ASIO_DECL)
# define BOOST_ASIO_DECL
#endif // !defined(BOOST_ASIO_DECL)

// Windows: target OS version.
#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)
# if !defined(_WIN32_WINNT) && !defined(_WIN32_WINDOWS)
#  if defined(_MSC_VER) || defined(__BORLANDC__)
#   pragma message( \
  "Please define _WIN32_WINNT or _WIN32_WINDOWS appropriately. For example:\n"\
  "- add -D_WIN32_WINNT=0x0501 to the compiler command line; or\n"\
  "- add _WIN32_WINNT=0x0501 to your project's Preprocessor Definitions.\n"\
  "Assuming _WIN32_WINNT=0x0501 (i.e. Windows XP target).")
#  else // defined(_MSC_VER) || defined(__BORLANDC__)
#   warning Please define _WIN32_WINNT or _WIN32_WINDOWS appropriately.
#   warning For example, add -D_WIN32_WINNT=0x0501 to the compiler command line.
#   warning Assuming _WIN32_WINNT=0x0501 (i.e. Windows XP target).
#  endif // defined(_MSC_VER) || defined(__BORLANDC__)
#  define _WIN32_WINNT 0x0501
# endif // !defined(_WIN32_WINNT) && !defined(_WIN32_WINDOWS)
# if defined(_MSC_VER)
#  if defined(_WIN32) && !defined(WIN32)
#   if !defined(_WINSOCK2API_)
#    define WIN32 // Needed for correct types in winsock2.h
#   else // !defined(_WINSOCK2API_)
#    error Please define the macro WIN32 in your compiler options
#   endif // !defined(_WINSOCK2API_)
#  endif // defined(_WIN32) && !defined(WIN32)
# endif // defined(_MSC_VER)
# if defined(__BORLANDC__)
#  if defined(__WIN32__) && !defined(WIN32)
#   if !defined(_WINSOCK2API_)
#    define WIN32 // Needed for correct types in winsock2.h
#   else // !defined(_WINSOCK2API_)
#    error Please define the macro WIN32 in your compiler options
#   endif // !defined(_WINSOCK2API_)
#  endif // defined(__WIN32__) && !defined(WIN32)
# endif // defined(__BORLANDC__)
# if defined(__CYGWIN__)
#  if !defined(__USE_W32_SOCKETS)
#   error You must add -D__USE_W32_SOCKETS to your compiler options.
#  endif // !defined(__USE_W32_SOCKETS)
# endif // defined(__CYGWIN__)
#endif // defined(BOOST_WINDOWS) || defined(__CYGWIN__)

// Windows: minimise header inclusion.
#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)
# if !defined(BOOST_ASIO_NO_WIN32_LEAN_AND_MEAN)
#  if !defined(WIN32_LEAN_AND_MEAN)
#   define WIN32_LEAN_AND_MEAN
#  endif // !defined(WIN32_LEAN_AND_MEAN)
# endif // !defined(BOOST_ASIO_NO_WIN32_LEAN_AND_MEAN)
#endif // defined(BOOST_WINDOWS) || defined(__CYGWIN__)

// Windows: suppress definition of "min" and "max" macros.
#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)
# if !defined(BOOST_ASIO_NO_NOMINMAX)
#  if !defined(NOMINMAX)
#   define NOMINMAX 1
#  endif // !defined(NOMINMAX)
# endif // !defined(BOOST_ASIO_NO_NOMINMAX)
#endif // defined(BOOST_WINDOWS) || defined(__CYGWIN__)

// Windows: IO Completion Ports.
#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)
# if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0400)
#  if !defined(UNDER_CE)
#   if !defined(BOOST_ASIO_DISABLE_IOCP)
#    define BOOST_ASIO_HAS_IOCP 1
#   endif // !defined(BOOST_ASIO_DISABLE_IOCP)
#  endif // !defined(UNDER_CE)
# endif // defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0400)
#endif // defined(BOOST_WINDOWS) || defined(__CYGWIN__)

// Linux: epoll, eventfd and timerfd.
#if defined(__linux__)
# include <linux/version.h>
# if !defined(BOOST_ASIO_DISABLE_EPOLL)
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,45)
#   define BOOST_ASIO_HAS_EPOLL 1
#  endif // LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,45)
# endif // !defined(BOOST_ASIO_DISABLE_EVENTFD)
# if !defined(BOOST_ASIO_DISABLE_EVENTFD)
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#   define BOOST_ASIO_HAS_EVENTFD 1
#  endif // LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
# endif // !defined(BOOST_ASIO_DISABLE_EVENTFD)
# if defined(BOOST_ASIO_HAS_EPOLL)
#  if (__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 8)
#   define BOOST_ASIO_HAS_TIMERFD 1
#  endif // (__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 8)
# endif // defined(BOOST_ASIO_HAS_EPOLL)
#endif // defined(__linux__)

// Mac OS X, FreeBSD, NetBSD, OpenBSD: kqueue.
#if (defined(__MACH__) && defined(__APPLE__)) \
  || defined(__FreeBSD__) \
  || defined(__NetBSD__) \
  || defined(__OpenBSD__)
# if !defined(BOOST_ASIO_DISABLE_KQUEUE)
#  define BOOST_ASIO_HAS_KQUEUE 1
# endif // !defined(BOOST_ASIO_DISABLE_KQUEUE)
#endif // (defined(__MACH__) && defined(__APPLE__))
       //   || defined(__FreeBSD__)
       //   || defined(__NetBSD__)
       //   || defined(__OpenBSD__)

// Solaris: /dev/poll.
#if defined(__sun)
# if !defined(BOOST_ASIO_DISABLE_DEV_POLL)
#  define BOOST_ASIO_HAS_DEV_POLL 1
# endif // !defined(BOOST_ASIO_DISABLE_DEV_POLL)
#endif // defined(__sun)

// Serial ports.
#if defined(BOOST_ASIO_HAS_IOCP) \
   || !defined(BOOST_WINDOWS) && !defined(__CYGWIN__)
# if !defined(__SYMBIAN32__)
#  if !defined(BOOST_ASIO_DISABLE_SERIAL_PORT)
#   define BOOST_ASIO_HAS_SERIAL_PORT 1
#  endif // !defined(BOOST_ASIO_DISABLE_SERIAL_PORT)
# endif // !defined(__SYMBIAN32__)
#endif // defined(BOOST_ASIO_HAS_IOCP)
       //   || !defined(BOOST_WINDOWS) && !defined(__CYGWIN__)

// Windows: stream handles.
#if !defined(BOOST_ASIO_DISABLE_WINDOWS_STREAM_HANDLE)
# if defined(BOOST_ASIO_HAS_IOCP)
#  define BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE 1
# endif // defined(BOOST_ASIO_HAS_IOCP)
#endif // !defined(BOOST_ASIO_DISABLE_WINDOWS_STREAM_HANDLE)

// Windows: random access handles.
#if !defined(BOOST_ASIO_DISABLE_WINDOWS_RANDOM_ACCESS_HANDLE)
# if defined(BOOST_ASIO_HAS_IOCP)
#  define BOOST_ASIO_HAS_WINDOWS_RANDOM_ACCESS_HANDLE 1
# endif // defined(BOOST_ASIO_HAS_IOCP)
#endif // !defined(BOOST_ASIO_DISABLE_WINDOWS_RANDOM_ACCESS_HANDLE)

// Windows: OVERLAPPED wrapper.
#if !defined(BOOST_ASIO_DISABLE_WINDOWS_OVERLAPPED_PTR)
# if defined(BOOST_ASIO_HAS_IOCP)
#  define BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR 1
# endif // defined(BOOST_ASIO_HAS_IOCP)
#endif // !defined(BOOST_ASIO_DISABLE_WINDOWS_OVERLAPPED_PTR)

// POSIX: stream-oriented file descriptors.
#if !defined(BOOST_ASIO_DISABLE_POSIX_STREAM_DESCRIPTOR)
# if !defined(BOOST_WINDOWS) && !defined(__CYGWIN__)
#  define BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR 1
# endif // !defined(BOOST_WINDOWS) && !defined(__CYGWIN__)
#endif // !defined(BOOST_ASIO_DISABLE_POSIX_STREAM_DESCRIPTOR)

// UNIX domain sockets.
#if !defined(BOOST_ASIO_DISABLE_LOCAL_SOCKETS)
# if !defined(BOOST_WINDOWS) && !defined(__CYGWIN__)
#  define BOOST_ASIO_HAS_LOCAL_SOCKETS 1
# endif // !defined(BOOST_WINDOWS) && !defined(__CYGWIN__)
#endif // !defined(BOOST_ASIO_DISABLE_LOCAL_SOCKETS)

#endif // BOOST_ASIO_DETAIL_CONFIG_HPP
