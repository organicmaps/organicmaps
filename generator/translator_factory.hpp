#pragma once

#include "generator/factory_utils.hpp"
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
  PLANET,
  REGION
};

template <class... Args>
std::shared_ptr<TranslatorInterface> CreateTranslator(TranslatorType type, Args&&... args)
{
  switch (type)
  {
  case TranslatorType::PLANET:
    return create<TranslatorPlanet>(std::forward<Args>(args)...);
  case TranslatorType::REGION:
    return create<TranslatorRegion>(std::forward<Args>(args)...);
  }
  CHECK_SWITCH();
}
}  // namespace generator
