#pragma once

#include "coding/internal/xmlparser.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"


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

namespace
{

template <class SourceT> class SequenceAdapter
{
  SourceT & m_source;
public:
  SequenceAdapter(SourceT & source) : m_source(source) {}
  uint64_t Read(void * p, uint64_t size)
  {
    size_t const correctSize = static_cast<size_t>(min(size, m_source.Size()));
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
