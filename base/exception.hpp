#pragma once

#include "base/internal/message.hpp"
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

#define DECLARE_EXCEPTION(exception_name, base_exception) \
  class exception_name : public base_exception \
  { \
  public: \
    exception_name(char const * what, std::string const & msg) : base_exception(what, msg) {} \
  }

// TODO: Use SRC_LOGGING macro.
#define MYTHROW(exception_name, msg) throw exception_name( \
  #exception_name " " __FILE__ ":" TO_STRING(__LINE__), ::base::Message msg)

#define MYTHROW1(exception_name, param1, msg) throw exception_name(param1, \
  #exception_name " " __FILE__ ":" TO_STRING(__LINE__), ::base::Message msg)
