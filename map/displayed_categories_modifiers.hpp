#pragma once

#include "search/displayed_categories.hpp"

#include <cstdint>
#include <unordered_set>

class SponsoredCategoryModifier : public search::CategoriesModifier
{
public:
  using SupportedCities = std::unordered_set<std::string>;

  SponsoredCategoryModifier(std::string const & currentCity,
                            SupportedCities const & supportedCities,
                            std::string const & categoryName,
                            uint32_t position);

  // CategoriesModifier overrides:
  void Modify(search::DisplayedCategories::Keys & keys) override;

private:
  std::string const m_currentCity;
  SupportedCities const m_supportedCities;
  std::string const m_categoryName;
  uint32_t const m_position;
};
