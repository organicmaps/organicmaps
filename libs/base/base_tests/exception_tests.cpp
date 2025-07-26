#include "testing/testing.hpp"

#include "base/exception.hpp"

#include <exception>
#include <string>

namespace
{
int FuncDoesNotThrow() noexcept
{
  return 1;
}
int FuncThrowsRootException()
{
  throw RootException("RootException", "RootException");
}
int FuncThrowsException()
{
  throw std::exception();
}
int FuncThrowsRuntimeError()
{
  throw std::runtime_error("runtime_error");
}
int FuncThrowsNumber()
{
  throw 1;
}

int FuncDoesNotThrowArg(int) noexcept
{
  return 1;
}
int FuncThrowsRootExceptionArg(int)
{
  throw RootException("RootException", "RootException");
}
int FuncThrowsExceptionArg(int)
{
  throw std::exception();
}
int FuncThrowsNumberArg(int)
{
  throw 1;
}

void FuncDoesNotThrowVoid(int) noexcept
{
  return;
}
void FuncThrowsRootExceptionVoid(int)
{
  throw RootException("RootException", "RootException");
}
void FuncThrowsExceptionVoid()
{
  throw std::exception();
}
void FuncThrowsNumberVoid()
{
  throw 1;
}

std::string const & ReturnsByRef(std::string const & str)
{
  bool exception = true;
  return ExceptionCatcher("ReturnsByRef().", exception,
                          [](std::string const & str) -> std::string const & { return str; }, str);
}

UNIT_TEST(ExceptionCatcher_FunctionsWithoutArgs)
{
  bool exception = false;
  ExceptionCatcher("Function without arg.", exception, FuncDoesNotThrow);
  TEST(!exception, ());
  ExceptionCatcher("Function without arg.", exception, FuncThrowsRootException);
  TEST(exception, ());
  ExceptionCatcher("Function without arg.", exception, FuncThrowsException);
  TEST(exception, ());
  ExceptionCatcher("Function without arg.", exception, FuncThrowsRuntimeError);
  TEST(exception, ());
  ExceptionCatcher("Function without arg.", exception, FuncThrowsNumber);
  TEST(exception, ());
}

UNIT_TEST(ExceptionCatcher_Functions)
{
  bool exception = false;
  ExceptionCatcher("Function with arg.", exception, FuncDoesNotThrowArg, 7);
  TEST(!exception, ());
  ExceptionCatcher("Function with arg.", exception, FuncThrowsRootExceptionArg, 7);
  TEST(exception, ());
  ExceptionCatcher("Function with arg.", exception, FuncThrowsExceptionArg, 7);
  TEST(exception, ());
  ExceptionCatcher("Function with arg.", exception, FuncThrowsNumberArg, 7);
  TEST(exception, ());
}

UNIT_TEST(ExceptionCatcher_LambdasReturnInt)
{
  bool exception = false;
  size_t callCount = 0;
  ExceptionCatcher("Lambda", exception, [&callCount](int)
  {
    ++callCount;
    return 1;
  }, 7);
  TEST(!exception, ());
  TEST_EQUAL(callCount, 1, ());

  ExceptionCatcher("Lambda", exception, [&callCount](int) -> int
  {
    ++callCount;
    throw RootException("RootException", "RootException");
  }, 7);
  TEST(exception, ());
  TEST_EQUAL(callCount, 2, ());

  ExceptionCatcher("Lambda", exception, [&callCount](int) -> int
  {
    ++callCount;
    throw std::exception();
  }, 7);
  TEST(exception, ());
  TEST_EQUAL(callCount, 3, ());

  ExceptionCatcher("Lambda", exception, [&callCount](int) -> int
  {
    ++callCount;
    throw 1;
  }, 7);
  TEST(exception, ());
  TEST_EQUAL(callCount, 4, ());
}

UNIT_TEST(ExceptionCatcher_LambdasReturnVoid)
{
  bool exception = false;
  size_t callCount = 0;
  ExceptionCatcher("Lambda", exception, [&callCount](int) { ++callCount; }, 7);
  TEST(!exception, ());
  TEST_EQUAL(callCount, 1, ());

  ExceptionCatcher("Lambda", exception, [&callCount](int)
  {
    ++callCount;
    throw RootException("RootException", "RootException");
  }, 7);
  TEST(exception, ());
  TEST_EQUAL(callCount, 2, ());

  ExceptionCatcher("Lambda", exception, [&callCount](int)
  {
    ++callCount;
    throw std::exception();
  }, 7);
  TEST(exception, ());
  TEST_EQUAL(callCount, 3, ());

  ExceptionCatcher("Lambda", exception, [&callCount](int)
  {
    ++callCount;
    throw 1;
  }, 7);
  TEST(exception, ());
  TEST_EQUAL(callCount, 4, ());
}

UNIT_TEST(ExceptionCatcher_FunctionsReturnVoid)
{
  bool exception = false;
  ExceptionCatcher("Function returns void.", exception, FuncDoesNotThrowVoid, 7);
  ExceptionCatcher("Function returns void.", exception, FuncThrowsRootExceptionVoid, 7);
  ExceptionCatcher("Function returns void.", exception, FuncThrowsExceptionVoid);
  ExceptionCatcher("Function returns void.", exception, FuncThrowsNumberVoid);
}

UNIT_TEST(ExceptionCatcher_PreventReturningRefOnLocalTemporaryObj)
{
  std::string const str = "A string";
  auto const returnedStr = ReturnsByRef(str);
  TEST_EQUAL(str, returnedStr, ());
}
}  // namespace
