#pragma once

#include "generator/feature_builder.hpp"
#include "generator/feature_maker_base.hpp"
#include "generator/intermediate_data.hpp"

struct OsmElement;

namespace generator
{
// Class FeatureMakerSimple is class FeatureMakerBase implementation for simple features building.
// It is only trying to build a feature and does not filter features in any way,
// except for bad geometry. This class is suitable for most cases.
class FeatureMakerSimple: public FeatureMakerBase
{
public:
  using FeatureMakerBase::FeatureMakerBase;

  // FeatureMaker overrides:
  std::shared_ptr<FeatureMakerBase> Clone() const override;

protected:
  // FeatureMaker overrides:
  void ParseParams(FeatureBuilderParams & params, OsmElement & element) const override;

private:
  // FeatureMaker overrides:
  bool BuildFromNode(OsmElement & element, FeatureBuilderParams const & params) override;
  bool BuildFromWay(OsmElement & element, FeatureBuilderParams const & params) override;
  bool BuildFromRelation(OsmElement & element, FeatureBuilderParams const & params) override;
};

// The difference between class FeatureMakerSimple and class FeatureMaker is that
// class FeatureMaker processes the types more strictly.
class FeatureMaker : public FeatureMakerSimple
{
public:
  using FeatureMakerSimple::FeatureMakerSimple;

  // FeatureMaker overrides:
  std::shared_ptr<FeatureMakerBase> Clone() const override;

private:
  // FeatureMaker overrides:
  void ParseParams(FeatureBuilderParams & params, OsmElement & element) const override;
};
}  // namespace generator
