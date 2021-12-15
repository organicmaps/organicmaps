#pragma once

#include "generator/feature_builder.hpp"
#include "generator/feature_maker_base.hpp"
#include "generator/intermediate_data.hpp"


struct OsmElement;

namespace generator
{
// FeatureMakerSimple is suitable for most cases for simple features.
// It filters features for bad geometry only.
class FeatureMakerSimple: public FeatureMakerBase
{
public:
  using FeatureMakerBase::FeatureMakerBase;

  std::shared_ptr<FeatureMakerBase> Clone() const override;

protected:
  void ParseParams(FeatureBuilderParams & params, OsmElement & element) const override;

  /// @return Any origin mercator point (prefer nodes) that belongs to \a e.
  std::optional<m2::PointD> GetOrigin(OsmElement const & e) const;

  /// @return Mercator point from intermediate cache storage.
  std::optional<m2::PointD> ReadNode(uint64_t id) const;

private:
  bool BuildFromNode(OsmElement & element, FeatureBuilderParams const & params) override;
  bool BuildFromWay(OsmElement & element, FeatureBuilderParams const & params) override;
  bool BuildFromRelation(OsmElement & element, FeatureBuilderParams const & params) override;
};

// FeatureMaker additionally filters the types using feature::IsUsefulType.
class FeatureMaker : public FeatureMakerSimple
{
public:
  using FeatureMakerSimple::FeatureMakerSimple;

  std::shared_ptr<FeatureMakerBase> Clone() const override;

private:
  void ParseParams(FeatureBuilderParams & params, OsmElement & element) const override;
};
}  // namespace generator
