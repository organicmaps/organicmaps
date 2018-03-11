#pragma once

#include "base/logging.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#define XML_STATIC
#include "3party/expat/expat_impl.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

template <typename DispatcherT>
class XmlParser : public CExpatImpl< XmlParser<DispatcherT> >
{
  typedef CExpatImpl< XmlParser<DispatcherT> > BaseT;

public:
  XmlParser(DispatcherT & dispatcher, bool enableCharHandler = false)
    : m_depth(0), m_restrictDepth(static_cast<size_t>(-1)), m_dispatcher(dispatcher),
    m_enableCharHandler(enableCharHandler)
  {
  }

  // Invoked by CExpatImpl after the parser is created
  void OnPostCreate()
  {
    // Enable all the event routines we want
    BaseT::EnableElementHandler();
    if (m_enableCharHandler)
      BaseT::EnableCharacterDataHandler();
  }

  // Start element handler
  void OnStartElement(XML_Char const * pszName, XML_Char const ** papszAttrs)
  {
    CheckCharData();

    ++m_depth;
    if (m_depth >= m_restrictDepth)
      return;

    if (!m_dispatcher.Push(pszName))
    {
      m_restrictDepth = m_depth;
      return;
    }

    for (size_t i = 0; papszAttrs[2 * i]; ++i)
      m_dispatcher.AddAttr(papszAttrs[2 * i], papszAttrs[2 * i + 1]);
  }

  // End element handler
  void OnEndElement(XML_Char const * pszName)
  {
    CheckCharData();

    --m_depth;
    if (m_depth >= m_restrictDepth)
      return;

    if (m_restrictDepth != size_t(-1))
      m_restrictDepth = static_cast<size_t>(-1);
    else
      m_dispatcher.Pop(std::string(pszName));
  }

  void OnCharacterData(XML_Char const * pszData, int nLength)
  {
    // Accumulate character data - it can be passed by parts
    // (when reading from fixed length buffer).
    m_charData.append(pszData, nLength);
  }

  void PrintError()
  {
    if (BaseT::GetErrorCode() != XML_ERROR_NONE)
    {
      LOG(LDEBUG, ("XML parse error at line",
                   BaseT::GetCurrentLineNumber(),
                   "and byte", BaseT::GetCurrentByteIndex()));
    }
  }

private:
  size_t m_depth;
  size_t m_restrictDepth;
  DispatcherT & m_dispatcher;

  std::string m_charData;
  bool m_enableCharHandler;

  void CheckCharData()
  {
    if (m_enableCharHandler && !m_charData.empty())
    {
      m_dispatcher.CharData(m_charData);
      m_charData.clear();
    }
  }
};
