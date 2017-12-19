#pragma once

#include "drape/pointers.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/spline.hpp"

#include <atomic>
#include <map>

namespace df
{
class MapDataProvider;

using MetalineCache = std::map<FeatureID, m2::SharedSpline>;

class ReadMetalineTask
{
public:
  ReadMetalineTask(MapDataProvider & model, MwmSet::MwmId const & mwmId);

  void Run();
  void Cancel();
  bool IsCancelled() const;

  MetalineCache const & GetMetalines() const { return m_metalines; }
  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }

private:
  MapDataProvider & m_model;
  MwmSet::MwmId m_mwmId;
  MetalineCache m_metalines;
  std::atomic<bool> m_isCancelled;
};
}  // namespace df
