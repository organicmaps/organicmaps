#include "base/src_point.hpp"

#include <algorithm>
#include <sstream>

namespace base
{
void SrcPoint::TruncateFileName()
{
  size_t const maxLen = 10000;
  char const * p[] = {m_fileName, m_fileName};
  for (size_t i = 0; i < maxLen && m_fileName[i]; ++i)
  {
    if (m_fileName[i] == '\\' || m_fileName[i] == '/')
    {
      std::swap(p[0], p[1]);
      p[0] = m_fileName + i + 1;
    }
  }
  m_fileName = p[1];
}

std::string DebugPrint(SrcPoint const & srcPoint)
{
  std::ostringstream out;
  if (srcPoint.Line() > 0)
    out << srcPoint.FileName() << ":" << srcPoint.Line() << " " << srcPoint.Function() << srcPoint.Postfix() << ": ";
  return out.str();
}
}  // namespace base
