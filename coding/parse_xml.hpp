#pragma once

#include "coding/internal/xmlparser.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>

DECLARE_EXCEPTION(XmlParseError, RootException);

template <typename Sequence, typename XMLDispatcher>
class XMLSequenceParser
{
public:
  XMLSequenceParser(Sequence & source, XMLDispatcher & dispatcher, bool useCharData = false)
    : m_res(0)
    , m_numRead(0)
    , m_source(source)
    , m_parser(dispatcher, useCharData)
  {}

  bool Read()
  {
    char * buffer = static_cast<char *>(m_parser.GetBuffer(kBufferSize));
    ASSERT(buffer, ());

    m_numRead = m_source.Read(buffer, kBufferSize);
    if (m_numRead == 0)
      return false;

    if (m_parser.ParseBuffer(static_cast<uint32_t>(m_numRead), false) == XML_STATUS_ERROR)
      MYTHROW(XmlParseError, (m_parser.GetErrorMessage()));

    m_res += m_numRead;
    return m_numRead == kBufferSize;
  }

private:
  uint32_t static const kBufferSize = 16 * 1024;

  uint64_t m_res = 0;
  uint64_t m_numRead = 0;
  Sequence & m_source;
  XmlParser<XMLDispatcher> m_parser;
};

template <class Source>
class SequenceAdapter
{
public:
  SequenceAdapter(Source & source) : m_source(source) {}

  uint64_t Read(void * p, uint64_t size)
  {
    size_t const correctSize = static_cast<size_t>(std::min(size, m_source.Size()));
    m_source.Read(p, correctSize);
    return correctSize;
  }

private:
  Source & m_source;
};

template <typename XMLDispatcher, typename Source>
bool ParseXML(Source & source, XMLDispatcher & dispatcher, bool useCharData = false)
{
  SequenceAdapter<Source> adapter(source);
  XMLSequenceParser<decltype(adapter), XMLDispatcher> parser(adapter, dispatcher, useCharData);
  try
  {
    while (parser.Read()) /* empty */
      ;
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, (e.what()));
    return false;
  }

  return true;
}
