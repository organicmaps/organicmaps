#include "geocoder/name_dictionary.hpp"

#include "base/assert.hpp"

#include <limits>
#include <utility>

namespace geocoder
{
// NameDictionary ----------------------------------------------------------------------------------
std::string const & NameDictionary::Get(Position position) const
{
  CHECK_GREATER(position, 0, ());
  CHECK_LESS_OR_EQUAL(position, m_stock.size(), ());
  return m_stock[position - 1];
}

NameDictionary::Position NameDictionary::Add(std::string const & s)
{
  CHECK_LESS(m_stock.size(), std::numeric_limits<uint32_t>::max(), ());
  m_stock.push_back(s);
  return m_stock.size();  // index + 1
}

// NameDictionaryBuilder -----------------------------------------------------------------------------
NameDictionary::Position NameDictionaryBuilder::Add(std::string const & s)
{
  auto indexItem = m_index.find(s);
  if (indexItem != m_index.end())
    return indexItem->second;

  auto p = m_dictionary.Add(s);
  auto indexEmplace = m_index.emplace(m_dictionary.Get(p), p);
  CHECK(indexEmplace.second, ());
  return p;
}

NameDictionary NameDictionaryBuilder::Release()
{
  m_index.clear();
  return std::move(m_dictionary);
}
}  // namespace geocoder
