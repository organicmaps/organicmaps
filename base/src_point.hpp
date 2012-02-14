#pragma once

#include "../std/algorithm.hpp"
#include "../std/sstream.hpp"
#include "../std/string.hpp"

#ifndef SRC_LOGGING
  #define SRC_LOGGING 1
#endif

#if SRC_LOGGING
  #ifndef __OBJC__
    #define SRC() my::SrcPoint(__FILE__, __LINE__, __FUNCTION__, "()")
  #else
    #define SRC() my::SrcPoint(__FILE__, __LINE__, __FUNCTION__)
  #endif
#else
  #define SRC() my::SrcPoint()
#endif

namespace my
{
  class SrcPoint
  {
  public:
    SrcPoint() : m_fileName(""), m_line(-1), m_function(""), m_postfix("")
    {
      TruncateFileName();
    }

    SrcPoint(char const * fileName, int line, char const * function, char const * postfix = "")
      : m_fileName(fileName), m_line(line), m_function(function), m_postfix(postfix)
    {
      TruncateFileName();
    }

    inline char const * FileName() const
    {
      return m_fileName;
    }

    inline int Line() const
    {
      return m_line;
    }

    inline char const * Function() const
    {
      return m_function;
    }

    inline char const * Postfix() const
    {
      return m_postfix;
    }

  private:
    void TruncateFileName()
    {
      size_t const maxLen = 10000;
      char const * p[] = { m_fileName, m_fileName };
      for (size_t i = 0; i < maxLen && m_fileName[i]; ++i)
      {
        if (m_fileName[i] == '\\' || m_fileName[i] == '/')
        {
          swap(p[0], p[1]);
          p[0] = m_fileName + i + 1;
        }
      }
      m_fileName = p[1];
    }

    char const * m_fileName;
    int m_line;
    char const * m_function;
    char const * m_postfix;
  };
}

inline string DebugPrint(my::SrcPoint const & srcPoint)
{
  ostringstream out;
  if (srcPoint.Line() > 0)
    out << srcPoint.FileName() << ":" << srcPoint.Line() << " " << srcPoint.Function()
        << srcPoint.Postfix() << " ";
  return out.str();
}
