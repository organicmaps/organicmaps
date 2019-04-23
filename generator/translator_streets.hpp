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
// The TranslatorStreets class implements translator for streets.
// Every Street is either a highway or a place=square.
class TranslatorStreets : public Translator
{
public:
  explicit TranslatorStreets(std::shared_ptr<EmitterInterface> emitter,
                             cache::IntermediateDataReader & cache);
};
}  // namespace generator
