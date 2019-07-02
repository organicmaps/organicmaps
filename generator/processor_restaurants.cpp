#include "generator/processor_restaurants.hpp"

#include "generator/feature_builder.hpp"

#include "indexer/ftypes_matcher.hpp"

namespace generator
{
ProcessorRestaurants::ProcessorRestaurants(std::shared_ptr<FeatureProcessorQueue> const & queue)
  : m_queue(queue)
{
  auto affilation = std::make_shared<feature::OneFileAffiliation>("");
  m_affilationsLayer = std::make_shared<AffilationsFeatureLayer<>>(kAffilationsBufferSize, affilation);
  m_processingChain->Add(m_affilationsLayer);
}

std::shared_ptr<FeatureProcessorInterface> ProcessorRestaurants::Clone() const
{
  return std::make_shared<ProcessorRestaurants>(m_queue);
}

void ProcessorRestaurants::Process(feature::FeatureBuilder & fb)
{
  if (!ftypes::IsEatChecker::Instance()(fb.GetParams().m_types) || fb.GetParams().name.IsEmpty())
  {
    ++m_stats.m_unexpectedFeatures;
    return;
  }

  switch (fb.GetGeomType())
  {
  case feature::GeomType::Point: ++m_stats.m_restaurantsPoi; break;
  case feature::GeomType::Area: ++m_stats.m_restaurantsBuilding; break;
  default: ++m_stats.m_unexpectedFeatures;
  }

  m_processingChain->Handle(fb);
  m_affilationsLayer->AddBufferToQueueIfFull(m_queue);
}

void ProcessorRestaurants::Flush()
{
  m_affilationsLayer->AddBufferToQueue(m_queue);
}

bool ProcessorRestaurants::Finish()
{
  LOG_SHORT(LINFO, ("Number of restaurants: POI:", m_stats.m_restaurantsPoi,
                    "BUILDING:", m_stats.m_restaurantsBuilding,
                    "INVALID:", m_stats.m_unexpectedFeatures));
  return true;
}

void ProcessorRestaurants::Merge(FeatureProcessorInterface const & other)
{
  other.MergeInto(*this);
}

void ProcessorRestaurants::MergeInto(ProcessorRestaurants & other) const
{
  other.m_processingChain->Merge(m_processingChain);
  other.m_stats = other.m_stats + m_stats;
}
}  // namespace generator
