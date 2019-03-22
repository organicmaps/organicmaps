#pragma once

#include "generator/feature_builder.hpp"

#include <queue>

struct OsmElement;

namespace generator
{
namespace cache
{
class IntermediateDataReader;
}  // namespace cache

// Abstract class FeatureMakerBase is responsible for the conversion OsmElement to FeatureBuilder1.
// The main task of this class is to create features of the necessary types.
// At least one feature should turn out from one OSM element. You can get several features from one element.
class FeatureMakerBase
{
public:
  explicit FeatureMakerBase(cache::IntermediateDataReader & holder);
  virtual ~FeatureMakerBase() = default;

  bool Add(OsmElement & p);
  // The function returns true when the receiving feature was successful and a false when not successful.
  bool GetNextFeature(FeatureBuilder1 & feature);
  size_t Size() const;
  bool Empty() const;

protected:
  virtual bool BuildFromNode(OsmElement & p, FeatureParams const & params) = 0;
  virtual bool BuildFromWay(OsmElement & p, FeatureParams const & params) = 0;
  virtual bool BuildFromRelation(OsmElement & p, FeatureParams const & params) = 0;

  virtual void ParseParams(FeatureParams & params, OsmElement & p) const  = 0;

  cache::IntermediateDataReader & m_holder;
  std::queue<FeatureBuilder1> m_queue;
};

void TransformAreaToPoint(FeatureBuilder1 & feature);
void TransformAreaToLine(FeatureBuilder1 & feature);

FeatureBuilder1 MakePointFromArea(FeatureBuilder1 const & feature);
FeatureBuilder1 MakeLineFromArea(FeatureBuilder1 const & feature);
}  // namespace generator
