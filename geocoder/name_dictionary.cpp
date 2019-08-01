#include "geocoder/name_dictionary.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <utility>

namespace geocoder
{
// MultipleName ------------------------------------------------------------------------------------
MultipleNames::MultipleNames(std::string const & mainName)
  : m_names{mainName}
{ }

std::string const & MultipleNames::GetMainName() const noexcept
{
  return m_names[0];
}

std::vector<std::string> const & MultipleNames::GetNames() const noexcept
{
  return m_names;
}

MultipleNames::const_iterator MultipleNames::begin() const noexcept
{
  return m_names.begin();
}

MultipleNames::const_iterator MultipleNames::end() const noexcept
{
  return m_names.end();
}

void MultipleNames::SetMainName(std::string const & name)
{
  m_names[0] = name;
}

void MultipleNames::AddAltName(std::string const & name)
{
  m_names.emplace_back(std::move(name));
  // Sort for operator==.
  ASSERT_GREATER_OR_EQUAL(m_names.size(), 2, ());
  std::inplace_merge(std::next(m_names.begin()), std::prev(m_names.end()), m_names.end());
}

bool operator==(MultipleNames const & lhs, MultipleNames const & rhs) noexcept
{
  return lhs.m_names == rhs.m_names;
}

bool operator!=(MultipleNames const & lhs, MultipleNames const & rhs) noexcept
{
  return !(lhs == rhs);
}

// NameDictionary ----------------------------------------------------------------------------------
MultipleNames const & NameDictionary::Get(Position position) const
{
  CHECK_GREATER(position, 0, ());
  CHECK_LESS_OR_EQUAL(position, m_stock.size(), ());
  return m_stock[position - 1];
}

NameDictionary::Position NameDictionary::Add(MultipleNames && names)
{
  CHECK(!names.GetMainName().empty(), ());
  CHECK_LESS(m_stock.size(), std::numeric_limits<uint32_t>::max(), ());
  m_stock.push_back(std::move(names));
  return m_stock.size();  // index + 1
}

// NameDictionaryBuilder::Hash ---------------------------------------------------------------------
size_t NameDictionaryBuilder::Hash::operator()(MultipleNames const & names) const noexcept
{
  return std::hash<std::string>{}(names.GetMainName());
}

// NameDictionaryBuilder -----------------------------------------------------------------------------
NameDictionary::Position NameDictionaryBuilder::Add(MultipleNames && names)
{
  auto indexItem = m_index.find(names);
  if (indexItem != m_index.end())
    return indexItem->second;

  auto p = m_dictionary.Add(std::move(names));
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
