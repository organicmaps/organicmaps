#pragma once

#include "std/cstdint.hpp"
#include "std/iostream.hpp"
#include "std/unique_ptr.hpp"

class Reader;
class Writer;


class BaseStreamBuf : public std::streambuf
{
public:
  typedef std::streambuf::traits_type traits_type;
  typedef std::streambuf::char_type char_type;
  typedef std::streambuf::int_type int_type;
};

class ReaderStreamBuf : public BaseStreamBuf
{
  unique_ptr<Reader> m_p;
  uint64_t m_pos, m_size;

public:
  ReaderStreamBuf(unique_ptr<Reader> && p);
  virtual ~ReaderStreamBuf();

private:
  virtual std::streamsize xsgetn(char_type * s, std::streamsize n);
  virtual int_type underflow();

  char m_buf[1];
};

class WriterStreamBuf : public BaseStreamBuf
{
  Writer * m_writer;

public:
  /// Takes the ownership of p. Writer should be allocated in dynamic memory.
  WriterStreamBuf(Writer * p) : m_writer(p) {}
  virtual ~WriterStreamBuf();

private:
  virtual std::streamsize xsputn(char_type const * s, std::streamsize n);
  virtual int_type overflow(int_type c);
  virtual int sync();
};
