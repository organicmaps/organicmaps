#pragma once

#include "search/displayed_categories.hpp"

class LuggageHeroModifier : public search::CategoriesModifier
{
public:
  explicit LuggageHeroModifier(std::string const & city);

  // CategoriesModifier overrides:
  void Modify(search::DisplayedCategories::Keys & keys) override;

private:
  std::string m_city;
};
