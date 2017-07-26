#pragma once
#include "base/base.hpp"
#include "base/internal/message.hpp"
#include "base/src_point.hpp"

#include <cassert>
#include <cstdlib>
#include <string>


namespace my
{
  // Called when ASSERT, CHECK or VERIFY failed.
  typedef void (*AssertFailedFn)(SrcPoint const &, std::string const &);
  extern AssertFailedFn OnAssertFailed;

  /// @return Pointer to previous message function.
  AssertFailedFn SetAssertFunction(AssertFailedFn fn);

  bool AssertAbortIsEnabled();
  void SwitchAssertAbort(bool enable);
}

#ifdef DEBUG
#define ABORT_ON_ASSERT() assert(false)
#else
#define ABORT_ON_ASSERT() std::abort()
#endif

#define ON_ASSERT_FAILED(msg)       \
  ::my::OnAssertFailed(SRC(), msg); \
  if (::my::AssertAbortIsEnabled()) \
    ABORT_ON_ASSERT();

// TODO: Evaluate X only once in CHECK().
#define CHECK(X, msg) do { if (X) {} else { \
  ON_ASSERT_FAILED(::my::impl::Message("CHECK("#X")", ::my::impl::Message msg));} } while(false)
#define CHECK_EQUAL(X, Y, msg) do { if ((X) == (Y)) {} else { \
  ON_ASSERT_FAILED(::my::impl::Message("CHECK("#X" == "#Y")", \
                                        ::my::impl::Message(X, Y), \
                                        ::my::impl::Message msg));} } while (false)
#define CHECK_NOT_EQUAL(X, Y, msg) do { if ((X) != (Y)) {} else { \
  ON_ASSERT_FAILED(::my::impl::Message("CHECK("#X" != "#Y")", \
                                        ::my::impl::Message(X, Y), \
                                        ::my::impl::Message msg));} } while (false)
#define CHECK_LESS(X, Y, msg) do { if ((X) < (Y)) {} else { \
  ON_ASSERT_FAILED(::my::impl::Message("CHECK("#X" < "#Y")", \
                                        ::my::impl::Message(X, Y), \
                                        ::my::impl::Message msg));} } while (false)
#define CHECK_LESS_OR_EQUAL(X, Y, msg) do { if ((X) <= (Y)) {} else { \
  ON_ASSERT_FAILED(::my::impl::Message("CHECK("#X" <= "#Y")", \
                                        ::my::impl::Message(X, Y), \
                                        ::my::impl::Message msg));} } while (false)
#define CHECK_GREATER(X, Y, msg) do { if ((X) > (Y)) {} else { \
  ON_ASSERT_FAILED(::my::impl::Message("CHECK("#X" > "#Y")", \
                                        ::my::impl::Message(X, Y), \
                                        ::my::impl::Message msg));} } while (false)
#define CHECK_GREATER_OR_EQUAL(X, Y, msg) do { if ((X) >= (Y)) {} else { \
  ON_ASSERT_FAILED(::my::impl::Message("CHECK("#X" >= "#Y")", \
                                        ::my::impl::Message(X, Y), \
                                        ::my::impl::Message msg));} } while (false)
#define CHECK_OR_CALL(fail, call, X, msg) do { if (X) {} else { \
  if (fail) {\
    ON_ASSERT_FAILED(::my::impl::Message(::my::impl::Message("CHECK("#X")"), \
                                         ::my::impl::Message msg)); \
  } else { \
    call(); \
  } } } while (false)

#ifdef DEBUG
// for Symbian compatibility
#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(X, msg) CHECK(X, msg)
#define VERIFY(X, msg) CHECK(X, msg)
#define ASSERT_EQUAL(X, Y, msg) CHECK_EQUAL(X, Y, msg)
#define ASSERT_NOT_EQUAL(X, Y, msg) CHECK_NOT_EQUAL(X, Y, msg)
#define ASSERT_LESS(X, Y, msg) CHECK_LESS(X, Y, msg)
#define ASSERT_LESS_OR_EQUAL(X, Y, msg) CHECK_LESS_OR_EQUAL(X, Y, msg)
#define ASSERT_GREATER(X, Y, msg) CHECK_GREATER(X, Y, msg)
#define ASSERT_GREATER_OR_EQUAL(X, Y, msg) CHECK_GREATER_OR_EQUAL(X, Y, msg)
#else
// for Symbian compatibility
#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(X, msg)
#define VERIFY(X, msg) (void)(X)
#define ASSERT_EQUAL(X, Y, msg)
#define ASSERT_NOT_EQUAL(X, Y, msg)
#define ASSERT_LESS(X, Y, msg)
#define ASSERT_LESS_OR_EQUAL(X, Y, msg)
#define ASSERT_GREATER(X, Y, msg)
#define ASSERT_GREATER_OR_EQUAL(X, Y, msg)
#endif
