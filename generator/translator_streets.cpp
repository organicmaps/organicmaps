#include "generator/translator_streets.hpp"

#include "generator/collector_interface.hpp"
#include "generator/feature_maker.hpp"
#include "generator/streets/streets_filter.hpp"
#include "generator/intermediate_data.hpp"

namespace generator
{
TranslatorStreets::TranslatorStreets(std::shared_ptr<FeatureProcessorInterface> const & processor,
                                     std::shared_ptr<cache::IntermediateData> const & cache)
  : Translator(processor, cache, std::make_shared<FeatureMakerSimple>(cache))

{
  SetFilter(std::make_shared<streets::StreetsFilter>());
}

std::shared_ptr<TranslatorInterface>
TranslatorStreets::Clone() const
{
  return Translator::CloneBase<TranslatorStreets>();
}

void TranslatorStreets::Merge(TranslatorInterface const & other)
{
  other.MergeInto(*this);
}

void TranslatorStreets::MergeInto(TranslatorStreets & other) const
{
  MergeIntoBase(other);
}
}  // namespace generator
