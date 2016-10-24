#pragma once
#include "base/internal/message.hpp"
#include "base/macros.hpp"

#include <exception>
#include <string>

class RootException : public std::exception
{
public:
  RootException(char const * what, std::string const & msg) : m_What(what), m_Msg(msg)
  {
  }

  virtual ~RootException() throw()
  {
  }

  virtual char const * what() const throw();

  std::string const & Msg() const throw()
  {
    return m_Msg;
  }

private:
  char const * m_What;
  std::string m_Msg;
};

#define DECLARE_EXCEPTION(exception_name, base_exception) \
  class exception_name : public base_exception \
  { \
  public: \
    exception_name(char const * what, std::string const & msg) : base_exception(what, msg) {} \
  }

// TODO: Use SRC_LOGGING macro.
#define MYTHROW(exception_name, msg) throw exception_name( \
  #exception_name " " __FILE__ ":" TO_STRING(__LINE__), ::my::impl::Message msg)

#define MYTHROW1(exception_name, param1, msg) throw exception_name(param1, \
  #exception_name " " __FILE__ ":" TO_STRING(__LINE__), ::my::impl::Message msg)
