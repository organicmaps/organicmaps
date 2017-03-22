#pragma once

#include "drape/pointers.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/spline.hpp"

#include "base/thread.hpp"
#include "base/thread_pool.hpp"

#include <map>

namespace df
{
class MapDataProvider;

using MetalineCache = std::map<FeatureID, m2::SharedSpline>;

class ReadMetalineTask : public threads::IRoutine
{
public:
  ReadMetalineTask(MapDataProvider & model);

  void Init(MwmSet::MwmId const & mwmId);

  void Do() override;
  void Reset() override;
  bool IsCancelled() const override;

  MetalineCache const & GetMetalines() const { return m_metalines; }
  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }

private:
  MapDataProvider & m_model;
  MwmSet::MwmId m_mwmId;
  MetalineCache m_metalines;
};

class ReadMetalineTaskFactory
{
public:
  ReadMetalineTaskFactory(MapDataProvider & model)
    : m_model(model)
  {}

  ReadMetalineTask * GetNew() const;

private:
  MapDataProvider & m_model;
};
}  // namespace df
