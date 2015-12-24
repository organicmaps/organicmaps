#pragma once

#include "indexer/features_vector.hpp"
#include "indexer/mwm_set.hpp"

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

  DISALLOW_COPY_AND_MOVE(MwmContext);
};
}  // namespace v2
}  // namespace search
