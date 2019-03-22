#pragma once

#include "generator/emitter_interface.hpp"
#include "generator/translator.hpp"

#include <memory>

namespace cache
{
class IntermediateDataReader;
}  // namespace cache

namespace generator
{
// The TranslatorGeoObjects class implements translator for building geo objects.
// Every GeoObject is either a building or a POI.
class TranslatorGeoObjects : public Translator
{
public:
  explicit TranslatorGeoObjects(std::shared_ptr<EmitterInterface> emitter,
                                cache::IntermediateDataReader & holder);
};
}  // namespace generator
