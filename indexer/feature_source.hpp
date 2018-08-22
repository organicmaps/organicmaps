#pragma once

#include "indexer/feature.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/rect2d.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

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
  explicit FeatureSource(MwmSet::MwmHandle const & handle);
  virtual ~FeatureSource() {}

  size_t GetNumFeatures() const;

  bool GetOriginalFeature(uint32_t index, FeatureType & feature) const;

  FeatureID GetFeatureId(uint32_t index) const { return FeatureID(m_handle.GetId(), index); }

  virtual FeatureStatus GetFeatureStatus(uint32_t index) const;

  virtual bool GetModifiedFeature(uint32_t index, FeatureType & feature) const;

  // Runs |fn| for each feature, that is not present in the mwm. 
  virtual void ForEachAdditionalFeature(m2::RectD const & rect, int scale,
                                        std::function<void(uint32_t)> const & fn) const;

protected:
  MwmSet::MwmHandle const & m_handle;
  std::unique_ptr<FeaturesVector> m_vector;
};  // class FeatureSource

// Lightweight FeatureSource factory. Each DataSource owns factory object.
class FeatureSourceFactory
{
public:
  virtual ~FeatureSourceFactory() = default;
  virtual std::unique_ptr<FeatureSource> operator()(MwmSet::MwmHandle const & handle) const
  {
    return std::make_unique<FeatureSource>(handle);
  }
};
