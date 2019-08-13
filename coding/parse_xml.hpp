#pragma once

#include "coding/internal/xmlparser.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <cstdint>

template <typename XMLDispatcherT, typename SequenceT>
uint64_t ParseXMLSequence(SequenceT & source, XMLDispatcherT & dispatcher, bool useCharData = false)
{
  // Create the parser
  XmlParser<XMLDispatcherT> parser(dispatcher, useCharData);
  if (!parser.Create())
    return 0;

  uint32_t const BUFFER_SIZE = 16 * 1024;

  uint64_t res = 0;
  uint64_t readed;
  do
  {
    char * buffer = static_cast<char *>(parser.GetBuffer(BUFFER_SIZE));
    ASSERT(buffer, ());

    readed = source.Read(buffer, BUFFER_SIZE);
    if (readed == 0)
      return res;

    if (!parser.ParseBuffer(static_cast<uint32_t>(readed), false))
    {
      parser.PrintError();
      return res;
    }

    res += readed;
  } while (readed == BUFFER_SIZE);

  return res;
}

template <typename SequenceT, typename XMLDispatcherT>
class ParserXMLSequence
{
public:
  ParserXMLSequence(SequenceT & source, XMLDispatcherT & dispatcher)
    : m_res(0)
    , m_readed(0)
    , m_source(source)
    , m_parser(dispatcher, false /* useCharData */)
  {
    CHECK(m_parser.Create(), ());

  }

  bool Read()
  {
    char * buffer = static_cast<char *>(m_parser.GetBuffer(kBufferSize));
    ASSERT(buffer, ());

    m_readed = m_source.Read(buffer, kBufferSize);
    if (m_readed == 0)
      return false;

    if (!m_parser.ParseBuffer(static_cast<uint32_t>(m_readed), false))
    {
      m_parser.PrintError();
      return false;
    }

    m_res += m_readed;
    return m_readed == kBufferSize;
  }

private:
  uint32_t static const kBufferSize = 16 * 1024;

  uint64_t m_res;
  uint64_t m_readed;
  SequenceT & m_source;
  XmlParser<XMLDispatcherT> m_parser;
};

namespace
{

template <class SourceT> class SequenceAdapter
{
  SourceT & m_source;
public:
  SequenceAdapter(SourceT & source) : m_source(source) {}
  uint64_t Read(void * p, uint64_t size)
  {
    size_t const correctSize = static_cast<size_t>(std::min(size, m_source.Size()));
    m_source.Read(p, correctSize);
    return correctSize;
  }
};

}

template <typename XMLDispatcherT, typename SourceT>
bool ParseXML(SourceT & source, XMLDispatcherT & dispatcher, bool useCharData = false)
{
  uint64_t const size = source.Size();
  SequenceAdapter<SourceT> adapter(source);
  return (ParseXMLSequence(adapter, dispatcher, useCharData) == size);
}
