#pragma once

#include "indexer/feature.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/mwm_set.hpp"

#include <functional>
#include <memory>
#include <string>

namespace datasource
{
enum class FeatureStatus
{
  Untouched,  // The feature hasn't been saved in the editor.
  Deleted,    // The feature has been marked as deleted.
  Obsolete,   // The feature has been marked for deletion via note.
  Modified,   // The feature has been saved in the editor and differs from the original one.
  Created     // The feature was created by a user and has been saved in the editor.
  // Note: If a feature was created by a user but hasn't been saved in the editor yet
  // its status is Untouched.
}; // enum class FeatureStatus

std::string ToString(FeatureStatus fs);
inline std::string DebugPrint(FeatureStatus fs) { return ToString(fs); }

class FeatureSource
{
public:
  FeatureSource(MwmSet::MwmHandle const & handle);

  size_t GetNumFeatures() const;

  bool GetOriginalFeature(uint32_t index, FeatureType & feature) const;

  inline FeatureID GetFeatureId(uint32_t index) const { return FeatureID(m_handle.GetId(), index); }

  virtual FeatureStatus GetFeatureStatus(uint32_t index) const;

  virtual bool GetModifiedFeature(uint32_t index, FeatureType & feature) const;

  virtual void ForEachInRectAndScale(m2::RectD const & rect, int scale,
                                     std::function<void(FeatureID const &)> const & fn);
  virtual void ForEachInRectAndScale(m2::RectD const & rect, int scale,
                                     std::function<void(FeatureType &)> const & fn);

protected:
  MwmSet::MwmHandle const & m_handle;
  std::unique_ptr<FeaturesVector> m_vector;
};  // class FeatureSource
}  // namespace datasource
