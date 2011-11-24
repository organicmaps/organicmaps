#pragma once

#include "../std/iostream.hpp"

class Reader;

class ReaderStreamBuf : public std::streambuf
{
  Reader * m_p;
  uint64_t m_pos, m_size;

public:
  typedef std::streambuf::traits_type traits_type;
  typedef std::streambuf::char_type char_type;
  typedef std::streambuf::int_type int_type;

  /// Takes the ownership of p. Reader should be allocated in dynamic memory.
  ReaderStreamBuf(Reader * p);
  virtual ~ReaderStreamBuf();

private:
  virtual std::streamsize xsgetn(char_type * s, std::streamsize n);
  virtual int_type underflow();

  char m_buf[1];
};
