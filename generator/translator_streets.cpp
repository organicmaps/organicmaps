#include "generator/translator_streets.hpp"

#include "generator/feature_maker.hpp"
#include "generator/streets/streets_filter.hpp"
#include "generator/intermediate_data.hpp"

namespace generator
{
TranslatorStreets::TranslatorStreets(std::shared_ptr<EmitterInterface> emitter,
                                     cache::IntermediateDataReader & cache)
  : Translator(emitter, cache, std::make_shared<FeatureMakerSimple>(cache))

{
  AddFilter(std::make_shared<streets::StreetsFilter>());
}
}  // namespace generator
