#pragma once

#include "platform/preferred_languages.hpp"

#include <string>
#include <vector>

namespace discovery
{
enum class ItemType
{
  Viator,
  Attractions,
  Cafe,
  Hotels,
  LocalExperts
};

using ItemTypes = std::vector<ItemType>;

struct ClientParams
{
  static auto constexpr kDefaultItemsCount = 5;

  std::string m_currency;
  std::string m_lang = languages::GetCurrentNorm();
  size_t m_itemsCount = kDefaultItemsCount;
  ItemTypes m_itemTypes;
};
}  // namespace discovery
