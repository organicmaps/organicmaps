#include "search/idf_map.hpp"

#include "base/assert.hpp"

namespace search
{
IdfMap::IdfMap(Delegate const & delegate, double unknownIdf) : m_delegate(delegate), m_unknownIdf(unknownIdf)
{
  ASSERT_GREATER(m_unknownIdf, 0.0, ());
}

double IdfMap::GetImpl(Map & idfs, strings::UniString const & s, bool isPrefix)
{
  auto const it = idfs.find(s);
  if (it != idfs.cend())
    return it->second;

  auto const df = static_cast<double>(m_delegate.GetNumDocs(s, isPrefix));
  auto const idf = df == 0 ? m_unknownIdf : 1.0 / df;
  idfs[s] = idf;

  return idf;
}
}  // namespace search
