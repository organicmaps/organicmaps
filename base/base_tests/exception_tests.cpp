#include "testing/testing.hpp"

#include "base/exception.hpp"

#include <exception>

namespace
{
int FuncDoesNotThrow() noexcept { return 1; }
int FuncThrowsRootException() { throw RootException("RootException", "RootException"); }
int FuncThrowsException() { throw std::exception(); }
int FuncThrowsNumber() { throw 1; };

int FuncDoesNotThrowArg(int) noexcept { return 1; }
int FuncThrowsRootExceptionArg(int) { throw RootException("RootException", "RootException"); }
int FuncThrowsExceptionArg(int) { throw std::exception(); }
int FuncThrowsNumberArg(int) { throw 1; };

UNIT_TEST(ExceptionCatcher_FunctionsWithoutArgs)
{
  ExceptionCatcher("ExceptionCatcher_Smoke", FuncDoesNotThrow);
  ExceptionCatcher("ExceptionCatcher_Smoke", FuncThrowsRootException);
  ExceptionCatcher("ExceptionCatcher_Smoke", FuncThrowsException);
  ExceptionCatcher("ExceptionCatcher_Smoke", FuncThrowsNumber);
}

UNIT_TEST(ExceptionCatcher_Functions)
{
  ExceptionCatcher("ExceptionCatcher", FuncDoesNotThrowArg, 7);
  ExceptionCatcher("ExceptionCatcher", FuncThrowsRootExceptionArg, 7);
  ExceptionCatcher("ExceptionCatcher", FuncThrowsExceptionArg, 7);
  ExceptionCatcher("ExceptionCatcher", FuncThrowsNumberArg, 7);
}

UNIT_TEST(ExceptionCatcher_Lambdas)
{
  ExceptionCatcher(
      "ExceptionCatcher", [](int) { return 1; }, 7);
  ExceptionCatcher(
      "ExceptionCatcher", [](int) -> int { throw RootException("RootException", "RootException"); },
      7);
  ExceptionCatcher(
      "ExceptionCatcher", [](int) -> int { throw std::exception(); }, 7);
  ExceptionCatcher(
      "ExceptionCatcher", [](int) -> int { throw 1; }, 7);
}
}  // namespace
