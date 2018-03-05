#pragma once

#include "coding/streams_common.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "std/type_traits.hpp"


namespace stream
{
  template <class TReader> class SinkReaderStream
  {
    TReader & m_reader;

  public:
    SinkReaderStream(TReader & reader) : m_reader(reader) {}

    template <typename T>
    enable_if_t<is_integral<T>::value, SinkReaderStream &> operator>>(T & t)
    {
      t = ReadPrimitiveFromSource<T>(m_reader);
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
      static_assert(sizeof(double) == sizeof(int64_t), "");
      int64_t * tInt = reinterpret_cast<int64_t *>(&t);
      operator >> (*tInt);
      return *this;
    }
  };

  template <class TWriter> class SinkWriterStream
  {
    TWriter & m_writer;

  public:
    SinkWriterStream(TWriter & writer) : m_writer(writer) {}

    template <typename T>
    enable_if_t<is_integral<T>::value, SinkWriterStream &> operator<<(T const & t)
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
      static_assert(sizeof(double) == sizeof(int64_t), "");
      int64_t const tInt = *reinterpret_cast<int64_t const *>(&t);
      operator << (tInt);
      return (*this);
    }
  };
}
