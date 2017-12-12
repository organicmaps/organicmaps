#include "search/idf_map.hpp"

namespace search
{
IdfMap::IdfMap(double unknownIdf): m_unknownIdf(unknownIdf) {}

double IdfMap::Get(strings::UniString const & s) const
{
  auto const it = m_idfs.find(s);
  return it == m_idfs.cend() ? m_unknownIdf : it->second;
}
}  // namespace search
