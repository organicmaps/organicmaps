#pragma once

#define XML_STATIC

#include "expat_impl.h"

#include "../../base/start_mem_debug.hpp"

template <typename DispatcherT>
class XmlParser : public CExpatImpl< XmlParser<DispatcherT> >
{
public:
  typedef CExpatImpl< XmlParser<DispatcherT> > BaseT;

  XmlParser(DispatcherT& dispatcher)
    : m_depth(0), m_restrictDepth(-1), m_dispatcher(dispatcher)
  {
  }

  // Invoked by CExpatImpl after the parser is created
  void OnPostCreate()
  {
    // Enable all the event routines we want
    BaseT::EnableElementHandler();
  }

  // Start element handler
  void OnStartElement(XML_Char const * pszName, XML_Char const ** papszAttrs)
  {
    ++m_depth;
    if (m_depth >= m_restrictDepth)
      return;

    if (!m_dispatcher.Push(pszName)) {
      m_restrictDepth = m_depth;
      return;
    }

    for (size_t i = 0; papszAttrs[2 * i]; ++i) {
      m_dispatcher.AddAttr(papszAttrs[2 * i], papszAttrs[2 * i + 1]);
    }
  }

  // End element handler
  void OnEndElement(XML_Char const * pszName)
  {
    --m_depth;
    if (m_depth >= m_restrictDepth)
      return;

    if (m_restrictDepth != size_t(-1)) {
      m_restrictDepth = -1;
    } else {
      m_dispatcher.Pop(string(pszName));
    }
  }

private:
  size_t m_depth;
  size_t m_restrictDepth;
  DispatcherT& m_dispatcher;
};

#include "../../base/stop_mem_debug.hpp"
