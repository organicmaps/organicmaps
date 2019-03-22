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
// The TranslatorCoastline class implements translator for building coastlines.
class TranslatorCoastline : public Translator
{
public:
  explicit TranslatorCoastline(std::shared_ptr<EmitterInterface> emitter,
                               cache::IntermediateDataReader & holder);
};
}  // namespace generator
