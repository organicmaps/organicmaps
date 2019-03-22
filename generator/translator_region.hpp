#pragma once

#include "generator/emitter_interface.hpp"
#include "generator/translator.hpp"

#include <memory>

namespace feature
{
struct GenerateInfo;
}  // namespace feature

namespace cache
{
class IntermediateDataReader;
}  // namespace cache

namespace generator
{
// TranslatorRegion class is responsible for processing regions.
class TranslatorRegion : public Translator
{
public:
  explicit TranslatorRegion(std::shared_ptr<EmitterInterface> emitter, cache::IntermediateDataReader & holder,
                            feature::GenerateInfo const & info);
};
}  // namespace generator
