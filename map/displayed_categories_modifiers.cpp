#include "map/displayed_categories_modifiers.hpp"

#include "base/macros.hpp"

#include <algorithm>

namespace
{
std::unordered_set<std::string> const kLuggageHeroesSupportedCities{"London", "New York",
                                                                    "Copenhagen"};

std::unordered_set<std::string> const kFc2018SupportedCities{
    "Moscow", "Saint Petersburg", "Kazan",         "Yekaterinburg", "Saransk",        "Samara",
    "Sochi",  "Volgograd",        "Rostov-on-Don", "Kaliningrad",   "Nizhny Novgorod"};
}  // namespace

SponsoredCategoryModifier::SponsoredCategoryModifier(std::string const & currentCity,
                                                     SupportedCities const & supportedCities,
                                                     std::string const & categoryName,
                                                     uint32_t position)
  : m_currentCity(currentCity)
  , m_supportedCities(supportedCities)
  , m_categoryName(categoryName)
  , m_position(position)
{}

void SponsoredCategoryModifier::Modify(search::DisplayedCategories::Keys & keys)
{
  auto const supported = m_supportedCities.find(m_currentCity) != m_supportedCities.cend();
  auto const contains = std::find(keys.cbegin(), keys.cend(), m_categoryName) != keys.cend();

  ASSERT_LESS(m_position, keys.size(), ());

  if (supported && !contains)
    keys.insert(keys.cbegin() + m_position, m_categoryName);
  else if (!supported && contains)
    keys.erase(std::remove(keys.begin(), keys.end(), m_categoryName), keys.end());
}

LuggageHeroModifier::LuggageHeroModifier(std::string const & currentCity)
  : SponsoredCategoryModifier(currentCity, kLuggageHeroesSupportedCities, "luggagehero",
                              4 /* position */)
{}

Fc2018Modifier::Fc2018Modifier(std::string const & currentCity)
  : SponsoredCategoryModifier(currentCity, kFc2018SupportedCities, "fc2018", 3 /* position */)
{}
