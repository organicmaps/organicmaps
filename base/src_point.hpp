#pragma once

#include <string>

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
  SrcPoint() : m_fileName(""), m_line(-1), m_function(""), m_postfix("") { TruncateFileName(); }
  SrcPoint(char const * fileName, int line, char const * function, char const * postfix = "")
    : m_fileName(fileName), m_line(line), m_function(function), m_postfix(postfix)
  {
    TruncateFileName();
  }

  inline char const * FileName() const { return m_fileName; }
  inline int Line() const { return m_line; }
  inline char const * Function() const { return m_function; }
  inline char const * Postfix() const { return m_postfix; }
private:
  void TruncateFileName();

  char const * m_fileName;
  int m_line;
  char const * m_function;
  char const * m_postfix;
};

std::string DebugPrint(SrcPoint const & srcPoint);
}
