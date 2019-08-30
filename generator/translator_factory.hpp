#pragma once

#include "generator/factory_utils.hpp"
#include "generator/translator_coastline.hpp"
#include "generator/translator_country.hpp"
#include "generator/translator_interface.hpp"
#include "generator/translator_world.hpp"

#include "base/assert.hpp"

#include <memory>
#include <utility>

namespace generator
{
enum class TranslatorType
{
  Country,
  CountryWithAds,
  Coastline,
  World,
  WorldWithAds
};

template <class... Args>
std::shared_ptr<TranslatorInterface> CreateTranslator(TranslatorType type, Args&&... args)
{
  switch (type)
  {
  case TranslatorType::Coastline:
    return create<TranslatorCoastline>(std::forward<Args>(args)...);
  case TranslatorType::Country:
    return create<TranslatorCountry>(std::forward<Args>(args)...);
  case TranslatorType::CountryWithAds:
    return create<TranslatorCountryWithAds>(std::forward<Args>(args)...);
  case TranslatorType::World:
    return create<TranslatorWorld>(std::forward<Args>(args)...);
  case TranslatorType::WorldWithAds:
    return create<TranslatorWorldWithAds>(std::forward<Args>(args)...);
  }
  UNREACHABLE();
}
}  // namespace generator
