#pragma once

#include "../base/base.hpp"
#include "../base/exception.hpp"


class SourceOutOfBoundsException : public RootException
{
public:
  SourceOutOfBoundsException(size_t bytesRead, char const * what, string const & msg)
    : RootException(what, msg), m_BytesRead(bytesRead)
  {
  }

  size_t BytesRead() const { return m_BytesRead; }

private:
  size_t const m_BytesRead;
};
