#pragma once

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"

#include <queue>

struct OsmElement;

namespace generator
{
// Abstract class FeatureMakerBase is responsible for the conversion OsmElement to FeatureBuilder.
// The main task of this class is to create features of the necessary types.
// At least one feature should turn out from one OSM element. You can get several features from one element.
class FeatureMakerBase
{
public:
  explicit FeatureMakerBase(
      std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache = {});
  virtual ~FeatureMakerBase() = default;

  virtual std::shared_ptr<FeatureMakerBase> Clone() const = 0;

  void SetCache(std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache);

  // Reference on element is non const because ftype::GetNameAndType will be call.
  virtual bool Add(OsmElement & element);
  // The function returns true when the receiving feature was successful and a false when not successful.
  bool GetNextFeature(feature::FeatureBuilder & feature);
  size_t Size() const;
  bool Empty() const;

protected:
  virtual bool BuildFromNode(OsmElement & element, FeatureBuilderParams const & params) = 0;
  virtual bool BuildFromWay(OsmElement & element, FeatureBuilderParams const & params) = 0;
  virtual bool BuildFromRelation(OsmElement & element, FeatureBuilderParams const & params) = 0;

  virtual void ParseParams(FeatureBuilderParams & params, OsmElement & element) const = 0;

  std::shared_ptr<cache::IntermediateDataReaderInterface> m_cache;
  std::queue<feature::FeatureBuilder> m_queue;
};

void TransformToPoint(feature::FeatureBuilder & feature);
void TransformToLine(feature::FeatureBuilder & feature);

feature::FeatureBuilder MakePoint(feature::FeatureBuilder const & feature);
feature::FeatureBuilder MakeLine(feature::FeatureBuilder const & feature);
}  // namespace generator
