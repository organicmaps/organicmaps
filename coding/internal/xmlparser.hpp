#pragma once

#include "base/assert.hpp"
#include "base/logging.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include <memory>
#include <sstream>
#include <string>

#define XML_STATIC
#include "3party/expat/expat/lib/expat.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

template <typename DispatcherT>
class XmlParser
{
public:
  explicit XmlParser(DispatcherT & dispatcher, bool enableCharHandler = false)
    : m_depth(0)
    , m_restrictDepth(static_cast<size_t>(-1))
    , m_dispatcher(dispatcher)
    , m_enableCharHandler(enableCharHandler)
    , m_parser(std::unique_ptr<XML_ParserStruct, decltype(&XML_ParserFree)>(
          XML_ParserCreate(nullptr /* encoding */), &XML_ParserFree))
  {
    CHECK(m_parser, ());
    OnPostCreate();
  }

  static void StartElementHandler(void * userData, XML_Char const * name,
                                  XML_Char const ** attrs)
  {
    CHECK(userData, (name));
    auto * xmlParser = static_cast<XmlParser *>(userData);
    xmlParser->OnStartElement(name, attrs);
  }

  static void EndElementHandler(void * userData, XML_Char const * name)
  {
    CHECK(userData, (name));
    auto * xmlParser = static_cast<XmlParser *>(userData);
    xmlParser->OnEndElement(name);
  }

  static void CharacterDataHandler(void * userData, XML_Char const * data, int length)
  {
    CHECK(userData, (data));
    auto * xmlParser = static_cast<XmlParser *>(userData);
    xmlParser->OnCharacterData(data, length);
  }

  void * GetBuffer(int len)
  {
    CHECK(m_parser, ());
    return XML_GetBuffer(m_parser.get(), len);
  }

  XML_Status ParseBuffer(int len, int isFinal)
  {
    CHECK(m_parser, ());
    return XML_ParseBuffer(m_parser.get(), len, isFinal);
  }

  void OnPostCreate()
  {
    CHECK(m_parser, ());
    // Enable all the event routines we want
    XML_SetStartElementHandler(m_parser.get(), StartElementHandler);
    XML_SetEndElementHandler(m_parser.get(), EndElementHandler);
    if (m_enableCharHandler)
      XML_SetCharacterDataHandler(m_parser.get(), CharacterDataHandler);

    XML_SetUserData(m_parser.get(), static_cast<void *>(this));
  }

  // Start element handler
  void OnStartElement(XML_Char const * name, XML_Char const ** attrs)
  {
    CheckCharData();

    ++m_depth;
    if (m_depth >= m_restrictDepth)
      return;

    if (!m_dispatcher.Push(name))
    {
      m_restrictDepth = m_depth;
      return;
    }

    for (size_t i = 0; attrs[2 * i]; ++i)
      m_dispatcher.AddAttr(attrs[2 * i], attrs[2 * i + 1]);
  }

  // End element handler
  void OnEndElement(XML_Char const * name)
  {
    CheckCharData();

    --m_depth;
    if (m_depth >= m_restrictDepth)
      return;

    if (m_restrictDepth != size_t(-1))
      m_restrictDepth = static_cast<size_t>(-1);
    else
      m_dispatcher.Pop(std::string(name));
  }

  void OnCharacterData(XML_Char const * data, int length)
  {
    // Accumulate character data - it can be passed by parts
    // (when reading from fixed length buffer).
    m_charData.append(data, length);
  }

  std::string GetErrorMessage()
  {
    if (XML_GetErrorCode(m_parser.get()) == XML_ERROR_NONE)
      return {};

    std::stringstream s;
    s << "XML parse error at line " << XML_GetCurrentLineNumber(m_parser.get())
      << " and byte " << XML_GetCurrentByteIndex(m_parser.get());
    return s.str();
  }

private:
  size_t m_depth;
  size_t m_restrictDepth;
  DispatcherT & m_dispatcher;

  std::string m_charData;
  bool m_enableCharHandler;
  std::unique_ptr<XML_ParserStruct, decltype(&XML_ParserFree)> m_parser;

  void CheckCharData()
  {
    if (m_enableCharHandler && !m_charData.empty())
    {
      m_dispatcher.CharData(m_charData);
      m_charData.clear();
    }
  }
};
