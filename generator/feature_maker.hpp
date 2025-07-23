#pragma once

#include "generator/feature_maker_base.hpp"

struct OsmElement;

namespace generator
{
// FeatureMakerSimple is suitable for most cases for simple features.
// It filters features for bad geometry only.
class FeatureMakerSimple : public FeatureMakerBase
{
public:
  using FeatureMakerBase::FeatureMakerBase;

  /// @name FeatureMakerBase overrides:
  /// @{
  std::shared_ptr<FeatureMakerBase> Clone() const override;

protected:
  void ParseParams(FeatureBuilderParams & params, OsmElement & element) const override;

  bool BuildFromNode(OsmElement & element, FeatureBuilderParams const & params) override;
  bool BuildFromWay(OsmElement & element, FeatureBuilderParams const & params) override;
  bool BuildFromRelation(OsmElement & element, FeatureBuilderParams const & params) override;
  /// @}

  /// @return Any origin mercator point (prefer nodes) that belongs to \a e.
  std::optional<m2::PointD> GetOrigin(OsmElement const & e) const;

  /// @return Mercator point from intermediate cache storage.
  std::optional<m2::PointD> ReadNode(uint64_t id) const;
};

// FeatureMaker additionally filters the types using feature::IsUsefulType.
class FeatureMaker : public FeatureMakerSimple
{
public:
  explicit FeatureMaker(IDRInterfacePtr const & cache = {});

  /// @name FeatureMakerBase overrides:
  /// @{
  std::shared_ptr<FeatureMakerBase> Clone() const override;

protected:
  void ParseParams(FeatureBuilderParams & params, OsmElement & element) const override;
  bool BuildFromRelation(OsmElement & p, FeatureBuilderParams const & params) override;
  /// @}

private:
  uint32_t m_placeClass;
};
}  // namespace generator
