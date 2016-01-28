#pragma once

#include "indexer/features_vector.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/scale_index.hpp"

#include "base/macros.hpp"

class MwmValue;

namespace search
{
namespace v2
{
struct MwmContext
{
  MwmContext(MwmSet::MwmHandle handle);

  MwmSet::MwmHandle m_handle;
  MwmValue & m_value;
  MwmSet::MwmId const & m_id;
  FeaturesVector m_vector;
  ScaleIndex<ModelReaderPtr> m_index;

  string const & GetName() const;
  shared_ptr<MwmInfo> const & GetInfo() const;

  DISALLOW_COPY_AND_MOVE(MwmContext);
};
}  // namespace v2
}  // namespace search
