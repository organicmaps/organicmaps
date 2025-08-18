#include "coding/reader_streambuf.hpp"
#include "coding/file_writer.hpp"
#include "coding/reader.hpp"

#include <algorithm>

ReaderStreamBuf::ReaderStreamBuf(std::unique_ptr<Reader> && p) : m_p(std::move(p)), m_pos(0), m_size(m_p->Size()) {}

// Define destructor in .cpp due to using unique_ptr with incomplete type.
ReaderStreamBuf::~ReaderStreamBuf() = default;

std::streamsize ReaderStreamBuf::xsgetn(char_type * s, std::streamsize n)
{
  std::streamsize const count = std::min(n, static_cast<std::streamsize>(m_size - m_pos));
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

std::streamsize WriterStreamBuf::xsputn(char_type const * s, std::streamsize n)
{
  m_writer.Write(s, n);
  return n;
}

WriterStreamBuf::int_type WriterStreamBuf::overflow(int_type c)
{
  if (!traits_type::eq_int_type(c, traits_type::eof()))
  {
    char_type const t = traits_type::to_char_type(c);
    xsputn(&t, 1);
  }
  return !traits_type::eof();
}

int WriterStreamBuf::sync()
{
  FileWriter * p = dynamic_cast<FileWriter *>(&m_writer);
  if (p)
    p->Flush();
  return 0;
}
