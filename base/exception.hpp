#pragma once

#include "base/internal/message.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

#include <exception>
#include <string>

class RootException : public std::exception
{
public:
  RootException(char const * what, std::string const & msg);

  virtual ~RootException() noexcept = default;

  std::string const & Msg() const { return m_msg; }

  // std::exception overrides:
  char const * what() const noexcept override { return m_whatWithAscii.c_str(); }

private:
  std::string m_whatWithAscii;
  std::string m_msg;
};

template <typename Fn, typename... Args>
std::invoke_result_t<Fn &&, Args &&...> ExceptionCatcher(std::string const & comment, bool & exceptionWasThrown,
                                                         Fn && fn, Args &&... args) noexcept
{
  try
  {
    exceptionWasThrown = false;
    return std::forward<Fn>(fn)(std::forward<Args>(args)...);
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("RootException.", comment, ex.Msg(), ex.what()));
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("std::exception.", comment, ex.what()));
  }
  catch (...)
  {
    LOG(LWARNING, ("Unknown exception.", comment));
  }

  exceptionWasThrown = true;
  using ReturnType = std::decay_t<std::invoke_result_t<Fn &&, Args &&...>>;
  if constexpr (!std::is_same_v<void, ReturnType>)
  {
    static ReturnType const defaultResult = {};
    return defaultResult;
  }
}

#define DECLARE_EXCEPTION(exception_name, base_exception)                                     \
  class exception_name : public base_exception                                                \
  {                                                                                           \
  public:                                                                                     \
    exception_name(char const * what, std::string const & msg) : base_exception(what, msg) {} \
  }

// TODO: Use SRC_LOGGING macro.
#define MYTHROW(exception_name, msg)                                                              \
  throw exception_name(#exception_name " " __FILE__ ":" TO_STRING(__LINE__), ::base::Message msg)

#define MYTHROW1(exception_name, param1, msg)                                                             \
  throw exception_name(param1, #exception_name " " __FILE__ ":" TO_STRING(__LINE__), ::base::Message msg)
