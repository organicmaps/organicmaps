#pragma once

#include "indexer/feature_decl.hpp"

#include "geometry/spline.hpp"

#include <atomic>
#include <map>

namespace df
{
uint8_t constexpr kMetaLinesSectionVersion = 3;

class MapDataProvider;

struct MetalineInfo
{
  m2::SharedSpline m_spline;
  double m_dashPhaseOffset = 0.0;
  bool m_dashPhaseReversed = false;
};

using MetalineCache = std::map<FeatureID, MetalineInfo>;

class ReadMetalineTask
{
public:
  ReadMetalineTask(MapDataProvider & model, MwmSet::MwmId const & mwmId);

  void Run();
  bool UpdateCache(MetalineCache & cache);

  void Cancel() { m_isCancelled = true; }
  bool IsCancelled() const { return m_isCancelled; }
  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }

private:
  MapDataProvider & m_model;
  MwmSet::MwmId m_mwmId;
  MetalineCache m_metalines;
  std::atomic<bool> m_isCancelled;
};
}  // namespace df
