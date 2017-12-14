#include "search/idf_map.hpp"

namespace search
{
IdfMap::IdfMap(Delegate & delegate, double unknownIdf)
  : m_delegate(delegate), m_unknownIdf(unknownIdf)
{
}

double IdfMap::Get(strings::UniString const & s)
{
  auto const it = m_idfs.find(s);
  if (it != m_idfs.cend())
    return it->second;

  auto const df = static_cast<double>(m_delegate.GetNumDocs(s));
  auto const idf = df == 0 ? m_unknownIdf : 1.0 / df;
  m_idfs[s] = idf;

  return idf;
}
}  // namespace search
