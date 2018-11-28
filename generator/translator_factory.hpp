#pragma once

#include "generator/factory_utils.hpp"
#include "generator/translator_geo_objects.hpp"
#include "generator/translator_interface.hpp"
#include "generator/translator_planet.hpp"
#include "generator/translator_region.hpp"

#include "base/assert.hpp"

#include <memory>
#include <utility>

namespace generator
{
enum class TranslatorType
{
  Planet,
  Region,
  GeoObjects
};

template <class... Args>
std::shared_ptr<TranslatorInterface> CreateTranslator(TranslatorType type, Args&&... args)
{
  switch (type)
  {
  case TranslatorType::Planet:
    return create<TranslatorPlanet>(std::forward<Args>(args)...);
  case TranslatorType::Region:
    return create<TranslatorRegion>(std::forward<Args>(args)...);
  case TranslatorType::GeoObjects:
    return create<TranslatorGeoObjects>(std::forward<Args>(args)...);
  }
  UNREACHABLE();
}
}  // namespace generator
