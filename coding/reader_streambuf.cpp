#include "reader_streambuf.hpp"
#include "reader.hpp"

#include "../std/algorithm.hpp"


ReaderStreamBuf::ReaderStreamBuf(Reader * p)
: m_p(p), m_pos(0), m_size(p->Size())
{
}

ReaderStreamBuf::~ReaderStreamBuf()
{
  delete m_p;
}

std::streamsize ReaderStreamBuf::xsgetn(char_type * s, std::streamsize n)
{
  uint64_t const count = min(static_cast<uint64_t>(n), m_size - m_pos);
  if (count > 0)
  {
    m_p->Read(m_pos, s, count);
    m_pos += count;
  }
  return count;
}

ReaderStreamBuf::int_type ReaderStreamBuf::underflow()
{
  std::streamsize s = xsgetn(m_buf, sizeof(m_buf));
  if (s > 0)
  {
    setg(m_buf, m_buf, m_buf + s);
    return traits_type::to_int_type(m_buf[0]);
  }
  else
  {
    return traits_type::eof();
  }
}
