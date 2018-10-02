#pragma once

#include "generator/translator_geocoder_base.hpp"

#include <memory>

namespace generator
{
// TranslatorRegion class is responsible for processing regions.
class TranslatorRegion : public TranslatorGeocoderBase
{
public:
  explicit TranslatorRegion(std::shared_ptr<EmitterInterface> emitter,
                            cache::IntermediateDataReader & holder,
                            std::shared_ptr<CollectorInterface> collector);

private:
  // TranslatorGeocoderBase overrides:
  bool IsSuitableElement(OsmElement const * p) const override;
};
}  // namespace generator
