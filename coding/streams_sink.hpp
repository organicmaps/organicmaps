#pragma once

#include "streams_common.hpp"
#include "reader.hpp"
#include "write_to_sink.hpp"


namespace stream
{
  template <class TReader> class SinkReaderStream
  {
    TReader & m_reader;

  public:
    SinkReaderStream(TReader & reader) : m_reader(reader) {}

    SinkReaderStream & operator >> (char & t)
    {
      t = ReadPrimitiveFromSource<char>(m_reader);
      return (*this);
    }
    SinkReaderStream & operator >> (uint64_t & t)
    {
      t = ReadPrimitiveFromSource<uint64_t>(m_reader);
      return (*this);
    }
    SinkReaderStream & operator >> (uint32_t & t)
    {
      t = ReadPrimitiveFromSource<uint32_t>(m_reader);
      return (*this);
    }
    SinkReaderStream & operator >> (uint16_t & t)
    {
      t = ReadPrimitiveFromSource<uint16_t>(m_reader);
      return (*this);
    }
    SinkReaderStream & operator >> (int64_t & t)
    {
      t = ReadPrimitiveFromSource<int64_t>(m_reader);
      return (*this);
    }
    SinkReaderStream & operator >> (int32_t & t)
    {
      t = ReadPrimitiveFromSource<int32_t>(m_reader);
      return (*this);
    }
    SinkReaderStream & operator >> (int16_t & t)
    {
      t = ReadPrimitiveFromSource<int16_t>(m_reader);
      return (*this);
    }

    SinkReaderStream & operator >> (bool & t)
    {
      detail::ReadBool(*this, t);
      return *this;
    }

    SinkReaderStream & operator >> (string & t)
    {
      detail::ReadString(*this, t);
      return *this;
    }

    SinkReaderStream & operator >> (double & t)
    {
      STATIC_ASSERT(sizeof(double) == sizeof(int64_t));
      operator>>(reinterpret_cast<int64_t &>(t));
      return *this;
    }

  };

  template <class TWriter> class SinkWriterStream
  {
    TWriter & m_writer;

  public:
    SinkWriterStream(TWriter & writer) : m_writer(writer) {}

    SinkWriterStream & operator << (char t)
    {
      WriteToSink(m_writer, t);
      return (*this);
    }
    SinkWriterStream & operator << (uint64_t t)
    {
      WriteToSink(m_writer, t);
      return (*this);
    }
    SinkWriterStream & operator << (uint32_t t)
    {
      WriteToSink(m_writer, t);
      return (*this);
    }
    SinkWriterStream & operator << (uint16_t t)
    {
      WriteToSink(m_writer, t);
      return (*this);
    }
    SinkWriterStream & operator << (int64_t t)
    {
      WriteToSink(m_writer, t);
      return (*this);
    }
    SinkWriterStream & operator << (int32_t t)
    {
      WriteToSink(m_writer, t);
      return (*this);
    }
    SinkWriterStream & operator << (int16_t t)
    {
      WriteToSink(m_writer, t);
      return (*this);
    }

    SinkWriterStream & operator << (bool t)
    {
      detail::WriteBool(*this, t);
      return (*this);
    }

    SinkWriterStream & operator << (string const & t)
    {
      detail::WriteString(*this, m_writer, t);
      return *this;
    }

    SinkWriterStream & operator << (double t)
    {
      STATIC_ASSERT(sizeof(double) == sizeof(int64_t));
      int64_t tInt = *reinterpret_cast<int64_t const *>(&t);
      operator<<(tInt);
      return (*this);
    }

  };
}
