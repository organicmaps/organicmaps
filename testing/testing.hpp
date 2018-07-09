#pragma once
#include "testing/testregister.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/src_point.hpp"

#include "std/iostream.hpp"
#include "std/string.hpp"


#define UNIT_TEST(name) \
    void UnitTest_##name();	\
        TestRegister g_TestRegister_##name(#name, __FILE__, &UnitTest_##name); \
    void UnitTest_##name()

#define UNIT_CLASS_TEST(CLASS, NAME)                \
  struct UnitClass_##CLASS##_##NAME : public CLASS  \
  {                                                 \
  public:                                           \
    void NAME();                                    \
  };                                                \
  UNIT_TEST(CLASS##_##NAME)                         \
  {                                                 \
    UnitClass_##CLASS##_##NAME instance;            \
    instance.NAME();                                \
  }                                                 \
  void UnitClass_##CLASS##_##NAME::NAME()

DECLARE_EXCEPTION(TestFailureException, RootException);

namespace my
{
  inline void OnTestFailed(SrcPoint const & srcPoint, string const & msg)
  {
    LOG(LINFO, ("FAILED"));
    LOG(LINFO, (::DebugPrint(srcPoint.FileName()) + ":" + ::DebugPrint(srcPoint.Line()), msg));
    MYTHROW(TestFailureException, (srcPoint.FileName(), srcPoint.Line(), msg));
  }
}

namespace testing
{
void RunEventLoop();
void StopEventLoop();

void Wait();
void Notify();
}  // namespace testing

// This struct contains parsed command line options. It may contain pointers to argc contents.
struct CommandLineOptions
{
  CommandLineOptions()
      : m_filterRegExp(nullptr), m_suppressRegExp(nullptr),
      m_dataPath(nullptr), m_resourcePath(nullptr), m_help(false)
  {
  }

  char const * m_filterRegExp;
  char const * m_suppressRegExp;
  char const * m_dataPath;
  char const * m_resourcePath;

  bool m_help;
  bool m_listTests;
};
CommandLineOptions const & GetTestingOptions();

#define TEST(X, msg)                                                                           \
  do                                                                                           \
  {                                                                                            \
    if (X)                                                                                     \
    {                                                                                          \
    }                                                                                          \
    else                                                                                       \
    {                                                                                          \
      ::my::OnTestFailed(SRC(), ::my::impl::Message("TEST(" #X ")", ::my::impl::Message msg)); \
    }                                                                                          \
  } while (0)
#define TEST_EQUAL(X, Y, msg)                                                                     \
  do                                                                                              \
  {                                                                                               \
    if ((X) == (Y))                                                                               \
    {                                                                                             \
    }                                                                                             \
    else                                                                                          \
    {                                                                                             \
      ::my::OnTestFailed(SRC(),                                                                   \
                         ::my::impl::Message("TEST(" #X " == " #Y ")", ::my::impl::Message(X, Y), \
                                             ::my::impl::Message msg));                           \
    }                                                                                             \
  } while (0)
#define TEST_NOT_EQUAL(X, Y, msg)                                                                 \
  do                                                                                              \
  {                                                                                               \
    if ((X) != (Y))                                                                               \
    {                                                                                             \
    }                                                                                             \
    else                                                                                          \
    {                                                                                             \
      ::my::OnTestFailed(SRC(),                                                                   \
                         ::my::impl::Message("TEST(" #X " != " #Y ")", ::my::impl::Message(X, Y), \
                                             ::my::impl::Message msg));                           \
    }                                                                                             \
  } while (0)
#define TEST_LESS(X, Y, msg)                                                                     \
  do                                                                                             \
  {                                                                                              \
    if ((X) < (Y))                                                                               \
    {                                                                                            \
    }                                                                                            \
    else                                                                                         \
    {                                                                                            \
      ::my::OnTestFailed(SRC(),                                                                  \
                         ::my::impl::Message("TEST(" #X " < " #Y ")", ::my::impl::Message(X, Y), \
                                             ::my::impl::Message msg));                          \
    }                                                                                            \
  } while (0)
#define TEST_LESS_OR_EQUAL(X, Y, msg)                                                             \
  do                                                                                              \
  {                                                                                               \
    if ((X) <= (Y))                                                                               \
    {                                                                                             \
    }                                                                                             \
    else                                                                                          \
    {                                                                                             \
      ::my::OnTestFailed(SRC(),                                                                   \
                         ::my::impl::Message("TEST(" #X " <= " #Y ")", ::my::impl::Message(X, Y), \
                                             ::my::impl::Message msg));                           \
    }                                                                                             \
  } while (0)
#define TEST_GREATER(X, Y, msg)                                                                  \
  do                                                                                             \
  {                                                                                              \
    if ((X) > (Y))                                                                               \
    {                                                                                            \
    }                                                                                            \
    else                                                                                         \
    {                                                                                            \
      ::my::OnTestFailed(SRC(),                                                                  \
                         ::my::impl::Message("TEST(" #X " > " #Y ")", ::my::impl::Message(X, Y), \
                                             ::my::impl::Message msg));                          \
    }                                                                                            \
  } while (0)
#define TEST_GREATER_OR_EQUAL(X, Y, msg)                                                          \
  do                                                                                              \
  {                                                                                               \
    if ((X) >= (Y))                                                                               \
    {                                                                                             \
    }                                                                                             \
    else                                                                                          \
    {                                                                                             \
      ::my::OnTestFailed(SRC(),                                                                   \
                         ::my::impl::Message("TEST(" #X " >= " #Y ")", ::my::impl::Message(X, Y), \
                                             ::my::impl::Message msg));                           \
    }                                                                                             \
  } while (0)
#define TEST_ALMOST_EQUAL_ULPS(X, Y, msg)                                                          \
  do                                                                                               \
  {                                                                                                \
    if (::my::AlmostEqualULPs(X, Y))                                                               \
    {                                                                                              \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      ::my::OnTestFailed(SRC(),                                                                    \
                         ::my::impl::Message("TEST(my::AlmostEqualULPs(" #X ", " #Y ")",           \
                                             ::my::impl::Message(X, Y), ::my::impl::Message msg)); \
    }                                                                                              \
  } while (0)
#define TEST_NOT_ALMOST_EQUAL_ULPS(X, Y, msg)                                                      \
  do                                                                                               \
  {                                                                                                \
    if (!::my::AlmostEqualULPs(X, Y))                                                              \
    {                                                                                              \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      ::my::OnTestFailed(SRC(),                                                                    \
                         ::my::impl::Message("TEST(!my::AlmostEqualULPs(" #X ", " #Y ")",          \
                                             ::my::impl::Message(X, Y), ::my::impl::Message msg)); \
    }                                                                                              \
  } while (0)

// TODO(AlexZ): Add more cool macroses (or switch all unit tests to gtest).
#define TEST_THROW(X, exception, msg)                                                       \
  do                                                                                        \
  {                                                                                         \
    bool expected_exception = false;                                                        \
    try                                                                                     \
    {                                                                                       \
      X;                                                                                    \
    }                                                                                       \
    catch (exception const &)                                                               \
    {                                                                                       \
      expected_exception = true;                                                            \
    }                                                                                       \
    catch (...)                                                                             \
    {                                                                                       \
      ::my::OnTestFailed(SRC(), ::my::impl::Message("Unexpected exception at TEST(" #X ")", \
                                                    ::my::impl::Message msg));              \
    }                                                                                       \
    if (!expected_exception)                                                                \
      ::my::OnTestFailed(SRC(), ::my::impl::Message("Expected exception " #exception        \
                                                    " was not thrown in TEST(" #X ")",      \
                                                    ::my::impl::Message msg));              \
  } while (0)
#define TEST_NO_THROW(X, msg)                                                               \
  do                                                                                        \
  {                                                                                         \
    try                                                                                     \
    {                                                                                       \
      X;                                                                                    \
    }                                                                                       \
    catch (...)                                                                             \
    {                                                                                       \
      ::my::OnTestFailed(SRC(), ::my::impl::Message("Unexpected exception at TEST(" #X ")", \
                                                    ::my::impl::Message msg));              \
    }                                                                                       \
  } while (0)
#define TEST_ANY_THROW(X, msg)                                                                   \
  do                                                                                             \
  {                                                                                              \
    bool was_exception = false;                                                                  \
    try                                                                                          \
    {                                                                                            \
      X;                                                                                         \
    }                                                                                            \
    catch (...)                                                                                  \
    {                                                                                            \
      was_exception = true;                                                                      \
    }                                                                                            \
    if (!was_exception)                                                                          \
      ::my::OnTestFailed(SRC(), ::my::impl::Message("No exceptions were thrown in TEST(" #X ")", \
                                                    ::my::impl::Message msg));                   \
  } while (0)
