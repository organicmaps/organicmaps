#pragma once

#include "base/base.hpp"
#include "base/internal/message.hpp"
#include "base/src_point.hpp"

#include <cassert>
#include <cstdlib>
#include <string>

// NOLINTBEGIN(misc-static-assert)

namespace base
{
// Called when ASSERT, CHECK or VERIFY failed.
// If returns true then crash application.
using AssertFailedFn = bool (*)(SrcPoint const &, std::string const &);
extern AssertFailedFn OnAssertFailed;

/// @return Pointer to previous message function.
AssertFailedFn SetAssertFunction(AssertFailedFn fn);
}  // namespace base

#ifdef DEBUG
#define ASSERT_CRASH() assert(false)
#else
#define ASSERT_CRASH() std::abort()
#endif

#define ASSERT_FAIL(msg)                  \
  if (::base::OnAssertFailed(SRC(), msg)) \
    ASSERT_CRASH();

// TODO: Evaluate X only once in CHECK().
#define CHECK(X, msg)                                                     \
  do                                                                      \
  {                                                                       \
    if (X)                                                                \
    {}                                                                    \
    else                                                                  \
    {                                                                     \
      ASSERT_FAIL(::base::Message("CHECK(" #X ")", ::base::Message msg)); \
    }                                                                     \
  }                                                                       \
  while (false)

#define CHECK_EQUAL(X, Y, msg)                                                                             \
  do                                                                                                       \
  {                                                                                                        \
    if ((X) == (Y))                                                                                        \
    {}                                                                                                     \
    else                                                                                                   \
    {                                                                                                      \
      ASSERT_FAIL(::base::Message("CHECK(" #X " == " #Y ")", ::base::Message(X, Y), ::base::Message msg)); \
    }                                                                                                      \
  }                                                                                                        \
  while (false)

#define CHECK_NOT_EQUAL(X, Y, msg)                                                                         \
  do                                                                                                       \
  {                                                                                                        \
    if ((X) != (Y))                                                                                        \
    {}                                                                                                     \
    else                                                                                                   \
    {                                                                                                      \
      ASSERT_FAIL(::base::Message("CHECK(" #X " != " #Y ")", ::base::Message(X, Y), ::base::Message msg)); \
    }                                                                                                      \
  }                                                                                                        \
  while (false)

#define CHECK_LESS(X, Y, msg)                                                                             \
  do                                                                                                      \
  {                                                                                                       \
    if ((X) < (Y))                                                                                        \
    {}                                                                                                    \
    else                                                                                                  \
    {                                                                                                     \
      ASSERT_FAIL(::base::Message("CHECK(" #X " < " #Y ")", ::base::Message(X, Y), ::base::Message msg)); \
    }                                                                                                     \
  }                                                                                                       \
  while (false)

#define CHECK_LESS_OR_EQUAL(X, Y, msg)                                                                     \
  do                                                                                                       \
  {                                                                                                        \
    if ((X) <= (Y))                                                                                        \
    {}                                                                                                     \
    else                                                                                                   \
    {                                                                                                      \
      ASSERT_FAIL(::base::Message("CHECK(" #X " <= " #Y ")", ::base::Message(X, Y), ::base::Message msg)); \
    }                                                                                                      \
  }                                                                                                        \
  while (false)

#define CHECK_GREATER(X, Y, msg)                                                                          \
  do                                                                                                      \
  {                                                                                                       \
    if ((X) > (Y))                                                                                        \
    {}                                                                                                    \
    else                                                                                                  \
    {                                                                                                     \
      ASSERT_FAIL(::base::Message("CHECK(" #X " > " #Y ")", ::base::Message(X, Y), ::base::Message msg)); \
    }                                                                                                     \
  }                                                                                                       \
  while (false)

#define CHECK_GREATER_OR_EQUAL(X, Y, msg)                                                                  \
  do                                                                                                       \
  {                                                                                                        \
    if ((X) >= (Y))                                                                                        \
    {}                                                                                                     \
    else                                                                                                   \
    {                                                                                                      \
      ASSERT_FAIL(::base::Message("CHECK(" #X " >= " #Y ")", ::base::Message(X, Y), ::base::Message msg)); \
    }                                                                                                      \
  }                                                                                                        \
  while (false)

#define CHECK_OR_CALL(fail, call, X, msg)                                                    \
  do                                                                                         \
  {                                                                                          \
    if (X)                                                                                   \
    {}                                                                                       \
    else                                                                                     \
    {                                                                                        \
      if (fail)                                                                              \
      {                                                                                      \
        ASSERT_FAIL(::base::Message(::base::Message("CHECK(" #X ")"), ::base::Message msg)); \
      }                                                                                      \
      else                                                                                   \
      {                                                                                      \
        call();                                                                              \
      }                                                                                      \
    }                                                                                        \
  }                                                                                          \
  while (false)

#ifdef DEBUG
#define ASSERT(X, msg)                     CHECK(X, msg)
#define VERIFY(X, msg)                     CHECK(X, msg)
#define ASSERT_EQUAL(X, Y, msg)            CHECK_EQUAL(X, Y, msg)
#define ASSERT_NOT_EQUAL(X, Y, msg)        CHECK_NOT_EQUAL(X, Y, msg)
#define ASSERT_LESS(X, Y, msg)             CHECK_LESS(X, Y, msg)
#define ASSERT_LESS_OR_EQUAL(X, Y, msg)    CHECK_LESS_OR_EQUAL(X, Y, msg)
#define ASSERT_GREATER(X, Y, msg)          CHECK_GREATER(X, Y, msg)
#define ASSERT_GREATER_OR_EQUAL(X, Y, msg) CHECK_GREATER_OR_EQUAL(X, Y, msg)
#else
#define ASSERT(X, msg)
#define VERIFY(X, msg) (void)(X)
#define ASSERT_EQUAL(X, Y, msg)
#define ASSERT_NOT_EQUAL(X, Y, msg)
#define ASSERT_LESS(X, Y, msg)
#define ASSERT_LESS_OR_EQUAL(X, Y, msg)
#define ASSERT_GREATER(X, Y, msg)
#define ASSERT_GREATER_OR_EQUAL(X, Y, msg)
#endif

// The macro that causes this warning to be ignored:
// "control reaches end of non-void function".
#define UNREACHABLE()                         \
  do                                          \
  {                                           \
    CHECK(false, ("Unreachable statement.")); \
    std::abort();                             \
  }                                           \
  while (false)

// NOLINTEND(misc-static-assert)
