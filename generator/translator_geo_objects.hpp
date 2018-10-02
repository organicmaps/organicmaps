#pragma once

#include "generator/translator_geocoder_base.hpp"

#include <memory>

namespace generator
{
// TranslatorGeoObjects class is responsible for processing geo objects.
// Every GeoObject is either a building or a POI.
class TranslatorGeoObjects : public TranslatorGeocoderBase
{
public:
  explicit TranslatorGeoObjects(std::shared_ptr<EmitterInterface> emitter,
                                cache::IntermediateDataReader & holder);

private:
  // TranslatorGeocoderBase overrides:
  bool IsSuitableElement(OsmElement const * p) const;
};
}  // namespace generator
