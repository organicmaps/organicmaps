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
  MwmContext(MwmValue & value, MwmSet::MwmId const & id);

  MwmValue & m_value;
  MwmSet::MwmId const m_id;
  FeaturesVector m_vector;

  DISALLOW_COPY_AND_MOVE(MwmContext);
};
}  // namespace v2
}  // namespace search
