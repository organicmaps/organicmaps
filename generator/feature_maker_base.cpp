#include "generator/feature_maker_base.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

#include "base/assert.hpp"

#include <utility>

using namespace feature;

namespace generator
{
FeatureMakerBase::FeatureMakerBase(
    std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache)
  : m_cache(cache)
{
}

void FeatureMakerBase::SetCache(
    std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache)
{
  m_cache = cache;
}

bool FeatureMakerBase::Add(OsmElement & element)
{
  ASSERT(m_cache, ());

  FeatureBuilderParams params;
  ParseParams(params, element);
  switch (element.m_type)
  {
  case OsmElement::EntityType::Node:
    return BuildFromNode(element, params);
  case OsmElement::EntityType::Way:
    return BuildFromWay(element, params);
  case OsmElement::EntityType::Relation:
    return BuildFromRelation(element, params);
  default:
    return false;
  }
}

size_t FeatureMakerBase::Size() const
{
  return m_queue.size();
}

bool FeatureMakerBase::Empty() const
{
  return m_queue.empty();
}

bool FeatureMakerBase::GetNextFeature(FeatureBuilder & feature)
{
  if (m_queue.empty())
    return false;

  feature = std::move(m_queue.front());
  m_queue.pop();
  return true;
}

void TransformToPoint(FeatureBuilder & feature)
{
  if (!feature.IsArea() && !feature.IsLine())
    return;

  auto const center = feature.GetGeometryCenter();
  feature.SetCenter(center);
  auto & params = feature.GetParams();
  if (!params.house.IsEmpty())
    params.SetGeomTypePointEx();
}

void TransformToLine(FeatureBuilder & feature)
{
  if (feature.IsLine())
    return;

  CHECK(feature.IsArea(), (feature));
  feature.SetLinear(feature.GetParams().GetReversedGeometry());
}

FeatureBuilder MakePoint(FeatureBuilder const & feature)
{
  FeatureBuilder tmp(feature);
  TransformToPoint(tmp);
  return tmp;
}

FeatureBuilder MakeLine(FeatureBuilder const & feature)
{
  FeatureBuilder tmp(feature);
  TransformToLine(tmp);
  return tmp;
}
}  // namespace generator
