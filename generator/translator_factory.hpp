#pragma once

#include "generator/factory_utils.hpp"
#include "generator/translator_coastline.hpp"
#include "generator/translator_country.hpp"
#include "generator/translator_geo_objects.hpp"
#include "generator/translator_interface.hpp"
#include "generator/translator_region.hpp"
#include "generator/translator_world.hpp"

#include "base/assert.hpp"

#include <memory>
#include <utility>

namespace generator
{
enum class TranslatorType
{
  Regions,
  GeoObjects,
  Country,
  Coastline,
  World
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
  case TranslatorType::Regions:
    return create<TranslatorRegion>(std::forward<Args>(args)...);
  case TranslatorType::GeoObjects:
    return create<TranslatorGeoObjects>(std::forward<Args>(args)...);
  case TranslatorType::World:
    return create<TranslatorWorld>(std::forward<Args>(args)...);
  }
  UNREACHABLE();
}
}  // namespace generator
