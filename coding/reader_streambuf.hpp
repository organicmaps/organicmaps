#pragma once

#include <cstdint>
#include <iostream>
#include <memory>

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
public:
  ReaderStreamBuf(std::unique_ptr<Reader> && p);
  virtual ~ReaderStreamBuf();

private:
  virtual std::streamsize xsgetn(char_type * s, std::streamsize n);
  virtual int_type underflow();

  std::unique_ptr<Reader> m_p;
  uint64_t m_pos = 0;
  uint64_t m_size = 0;
  char m_buf[1] = {};
};

class WriterStreamBuf : public BaseStreamBuf
{
  Writer & m_writer;

public:
  /// Takes the ownership of p. Writer should be allocated in dynamic memory.
  explicit WriterStreamBuf(Writer & writer) : m_writer(writer) {}

private:
  virtual std::streamsize xsputn(char_type const * s, std::streamsize n);
  virtual int_type overflow(int_type c);
  virtual int sync();
};
