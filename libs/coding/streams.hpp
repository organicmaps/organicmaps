#pragma once
#include "coding/streams_common.hpp"

namespace stream
{
template <class TReader>
class ReaderStream
{
  TReader & m_reader;

public:
  ReaderStream(TReader & reader) : m_reader(reader) {}

  ReaderStream & operator>>(char & t)
  {
    m_reader.Read(&t, sizeof(t));
    return (*this);
  }
  ReaderStream & operator>>(uint32_t & t)
  {
    m_reader.Read(&t, sizeof(t));
    return (*this);
  }
  ReaderStream & operator>>(int32_t & t)
  {
    m_reader.Read(&t, sizeof(t));
    return (*this);
  }
  ReaderStream & operator>>(uint64_t & t)
  {
    m_reader.Read(&t, sizeof(t));
    return (*this);
  }
  ReaderStream & operator>>(int64_t & t)
  {
    m_reader.Read(&t, sizeof(t));
    return (*this);
  }
  ReaderStream & operator>>(double & t)
  {
    m_reader.Read(&t, sizeof(t));
    return (*this);
  }

  ReaderStream & operator>>(bool & t)
  {
    detail::ReadBool(*this, t);
    return *this;
  }

  ReaderStream & operator>>(string & t)
  {
    detail::ReadString(*this, t);
    return *this;
  }
};

template <class TWriter>
class WriterStream
{
  TWriter & m_writer;

public:
  WriterStream(TWriter & writer) : m_writer(writer) {}

  WriterStream & operator<<(char t)
  {
    m_writer.Write(&t, sizeof(t));
    return (*this);
  }
  WriterStream & operator<<(uint64_t t)
  {
    m_writer.Write(&t, sizeof(t));
    return (*this);
  }
  WriterStream & operator<<(uint32_t t)
  {
    m_writer.Write(&t, sizeof(t));
    return (*this);
  }
  WriterStream & operator<<(int64_t t)
  {
    m_writer.Write(&t, sizeof(t));
    return (*this);
  }
  WriterStream & operator<<(int32_t t)
  {
    m_writer.Write(&t, sizeof(t));
    return (*this);
  }
  WriterStream & operator<<(double t)
  {
    m_writer.Write(&t, sizeof(t));
    return (*this);
  }
  WriterStream & operator<<(bool t)
  {
    detail::WriteBool(*this, t);
    return (*this);
  }

  WriterStream & operator<<(string const & t)
  {
    detail::WriteString(*this, m_writer, t);
    return *this;
  }
};
}  // namespace stream
