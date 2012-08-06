#pragma once

#include "internal/xmlparser.h"
#include "source.hpp"

#include "../base/assert.hpp"


static const size_t KMaxXMLFileBufferSize = 16384;

template <typename XMLDispatcherT, typename SourceT>
bool ParseXML(SourceT & source, XMLDispatcherT & dispatcher, bool useCharData = false)
{
  // Create the parser
  XmlParser<XMLDispatcherT> parser(dispatcher, useCharData);
  if (!parser.Create())
    return false;

  try
  {
    while (true)
    {
      char * buffer = static_cast<char *>(parser.GetBuffer(KMaxXMLFileBufferSize));
      CHECK(buffer, ());
      source.Read(buffer, KMaxXMLFileBufferSize);
      if (!parser.ParseBuffer(KMaxXMLFileBufferSize, false))
        return false;
    }
  }
  catch (SourceOutOfBoundsException & e)
  {
    size_t const toRead = e.BytesRead();
    // 0 - means Reader overflow (see ReaderSource::Read)
    if (toRead == 0 || !parser.ParseBuffer(toRead, true))
      return false;
  }

  return true;
}
