#include "testing/testing.hpp"

#include "base/exception.hpp"

#include <exception>

namespace
{
int FuncDoesNotThrow() noexcept { return 1; }
int FuncThrowsRootException() { throw RootException("RootException", "RootException"); }
int FuncThrowsException() { throw std::exception(); }
int FuncThrowsRuntimeError() { throw std::runtime_error("runtime_error"); }
int FuncThrowsNumber() { throw 1; };

int FuncDoesNotThrowArg(int) noexcept { return 1; }
int FuncThrowsRootExceptionArg(int) { throw RootException("RootException", "RootException"); }
int FuncThrowsExceptionArg(int) { throw std::exception(); }
int FuncThrowsNumberArg(int) { throw 1; };

void FuncDoesNotThrowVoid(int) noexcept { return; }
void FuncThrowsRootExceptionVoid(int) { throw RootException("RootException", "RootException"); }
void FuncThrowsExceptionVoid() { throw std::exception(); }
void FuncThrowsNumberVoid() { throw 1; };

UNIT_TEST(ExceptionCatcher_FunctionsWithoutArgs)
{
  ExceptionCatcher("Function without arg.", FuncDoesNotThrow);
  ExceptionCatcher("Function without arg.", FuncThrowsRootException);
  ExceptionCatcher("Function without arg.", FuncThrowsException);
  ExceptionCatcher("Function without arg.", FuncThrowsRuntimeError);
  ExceptionCatcher("Function without arg.", FuncThrowsNumber);
}

UNIT_TEST(ExceptionCatcher_Functions)
{
  ExceptionCatcher("Function with arg.", FuncDoesNotThrowArg, 7);
  ExceptionCatcher("Function with arg.", FuncThrowsRootExceptionArg, 7);
  ExceptionCatcher("Function with arg.", FuncThrowsExceptionArg, 7);
  ExceptionCatcher("Function with arg.", FuncThrowsNumberArg, 7);
}

UNIT_TEST(ExceptionCatcher_Lambdas)
{
  ExceptionCatcher(
      "Lambda", [](int) { return 1; }, 7);
  ExceptionCatcher(
      "Lambda", [](int) -> int { throw RootException("RootException", "RootException"); }, 7);
  ExceptionCatcher(
      "Lambda", [](int) -> int { throw std::exception(); }, 7);
  ExceptionCatcher(
      "Lambda", [](int) -> int { throw 1; }, 7);
}

UNIT_TEST(ExceptionCatcher_FunctionsReturnVoid)
{
  ExceptionCatcher("Function returns void.", FuncDoesNotThrowVoid, 7);
  ExceptionCatcher("Function returns void.", FuncThrowsRootExceptionVoid, 7);
  ExceptionCatcher("Function returns void.", FuncThrowsExceptionVoid);
  ExceptionCatcher("Function returns void..", FuncThrowsNumberVoid);
}
}  // namespace
