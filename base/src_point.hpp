#pragma once
#include "base.hpp"
#include "../std/algorithm.hpp"
#include "../std/sstream.hpp"
#include "../std/string.hpp"

#ifndef SRC_LOGGING
#define SRC_LOGGING 1
#endif

#if SRC_LOGGING
#define SRC() my::SrcPoint(__FILE__, __LINE__, __FUNCTION__)
#else
#define SRC() my::SrcPoint()
#endif

namespace my
{
  class SrcPoint
  {
  public:
    SrcPoint() : m_FileName("X"), m_Line(-1), m_Function("X")
    {
      TruncateFileName();
    }

    SrcPoint(char const * fileName, int line, char const * function)
      : m_FileName(fileName), m_Line(line), m_Function(function)
    {
      TruncateFileName();
    }

    inline char const * FileName() const
    {
      return m_FileName;
    }

    inline int Line() const
    {
      return m_Line;
    }

    inline char const * Function() const
    {
      return m_Function;
    }
  private:
    void TruncateFileName()
    {
      size_t const maxLen = 10000;
      char const * p[] = { m_FileName, m_FileName };
      for (size_t i = 0; i < maxLen && m_FileName[i]; ++i)
      {
        if (m_FileName[i] == '\\' || m_FileName[i] == '/')
        {
          swap(p[0], p[1]);
          p[0] = m_FileName + i + 1;
        }
      }
      m_FileName = p[1];
    }

    char const * m_FileName;
    int m_Line;
    char const * m_Function;
  };
}

inline string DebugPrint(my::SrcPoint const & srcPoint)
{
    ostringstream out;
    out << srcPoint.FileName() << ":" << srcPoint.Line() << " " << srcPoint.Function() << "()";
    return out.str();
}


