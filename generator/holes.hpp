#pragma once

#include "generator/feature_builder.hpp"
#include "generator/ways_merger.hpp"

#include "base/control_flow.hpp"

#include <cstdint>

struct OsmElement;

namespace generator
{
namespace cache
{
class IntermediateDataReaderInterface;
}  // namespace cache

class HolesAccumulator
{
public:
  explicit HolesAccumulator(std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache);

  void operator()(uint64_t id) { m_merger.AddWay(id); }
  feature::FeatureBuilder::Geometry & GetHoles();

private:
  AreaWayMerger m_merger;
  feature::FeatureBuilder::Geometry m_holes;
};

/// Find holes for way with 'id' in first relation.
class HolesProcessor
{
public:
  explicit HolesProcessor(uint64_t id, std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache);

  /// 1. relations process function
  base::ControlFlow operator()(uint64_t /* id */, RelationElement const & e);
  /// 2. "ways in relation" process function
  void operator()(uint64_t id, std::string const & role);
  feature::FeatureBuilder::Geometry & GetHoles() { return m_holes.GetHoles(); }

private:
  uint64_t m_id;  ///< id of way to find it's holes
  HolesAccumulator m_holes;
};

class HolesRelation
{
public:
  explicit HolesRelation(std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache);

  void Build(OsmElement const * p);
  feature::FeatureBuilder::Geometry & GetHoles() { return m_holes.GetHoles(); }
  AreaWayMerger & GetOuter() { return m_outer; }

private:
  HolesAccumulator m_holes;
  AreaWayMerger m_outer;
};
}  // namespace generator
