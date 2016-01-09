#pragma once

#include "geometry/rect2d.hpp"

#include "indexer/feature_meta.hpp"
#include "indexer/mwm_set.hpp"

#include "editor/xml_feature.hpp"

#include "base/timer.hpp"

#include "std/ctime.hpp"
#include "std/function.hpp"
#include "std/map.hpp"
#include "std/set.hpp"
#include "std/string.hpp"

struct FeatureID;
class FeatureType;
class Index;

namespace osm
{
class Editor final
{
  Editor() = default;

public:
  using TMwmIdByMapNameFn = function<MwmSet::MwmId(string const & /*map*/)>;
  using TInvalidateFn = function<void()>;
  using TFeatureLoaderFn = function<unique_ptr<FeatureType> (FeatureID const & /*fid*/)>;

  enum class FeatureStatus
  {
    Untouched,
    Deleted,
    Modified,
    Created
  };

  using TTypes = vector<uint32_t>;

  static Editor & Instance();

  void SetMwmIdByNameAndVersionFn(TMwmIdByMapNameFn const & fn) { m_mwmIdByMapNameFn = fn; }
  void SetInvalidateFn(TInvalidateFn const & fn) { m_invalidateFn = fn; }
  void SetFeatureLoaderFn(TFeatureLoaderFn const & fn) { m_featureLoaderFn = fn; }

  void LoadMapEdits();

  using TFeatureIDFunctor = function<void(FeatureID const &)>;
  using TFeatureTypeFunctor = function<void(FeatureType &)>;
  void ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                       TFeatureIDFunctor const & f,
                                       m2::RectD const & rect,
                                       uint32_t scale);
  void ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                       TFeatureTypeFunctor const & f,
                                       m2::RectD const & rect,
                                       uint32_t scale);

  /// Easy way to check if feature was deleted, modified, created or not changed at all.
  FeatureStatus GetFeatureStatus(MwmSet::MwmId const & mwmId, uint32_t offset) const;

  /// Marks feature as "deleted" from MwM file.
  void DeleteFeature(FeatureType const & feature);

  /// @returns false if feature wasn't edited.
  /// @param outFeature is valid only if true was returned.
  bool GetEditedFeature(MwmSet::MwmId const & mwmId, uint32_t offset, FeatureType & outFeature) const;

  /// Original feature with same FeatureID as newFeature is replaced by newFeature.
  void EditFeature(FeatureType & editedFeature);

  vector<feature::Metadata::EType> EditableMetadataForType(FeatureType const & feature) const;
  bool IsNameEditable(FeatureType const & feature) const;
  bool IsHouseNumberEditable(FeatureType const & feature) const;

private:
  // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
  void Save(string const & fullFilePath) const;

  struct FeatureTypeInfo
  {
    FeatureStatus m_status;
    FeatureType m_feature;
    time_t m_modificationTimestamp = my::INVALID_TIME_STAMP;
    time_t m_uploadAttemptTimestamp = my::INVALID_TIME_STAMP;
    /// "" | "ok" | "repeat" | "failed"
    string m_uploadStatus;
    string m_uploadError;
  };

  // TODO(AlexZ): Synchronize multithread access.
  /// Deleted, edited and created features.
  map<MwmSet::MwmId, map<uint32_t, FeatureTypeInfo>> m_features;

  /// Get MwmId for each map, used in FeatureIDs and to check if edits are up-to-date.
  TMwmIdByMapNameFn m_mwmIdByMapNameFn;
  /// Invalidate map viewport after edits.
  TInvalidateFn m_invalidateFn;
  /// Get FeatureType from mwm.
  TFeatureLoaderFn m_featureLoaderFn;
};  // class Editor

inline string DebugPrint(Editor::FeatureStatus fs)
{
  switch (fs)
  {
  case Editor::FeatureStatus::Untouched: return "Untouched";
  case Editor::FeatureStatus::Deleted: return "Deleted";
  case Editor::FeatureStatus::Modified: return "Modified";
  case Editor::FeatureStatus::Created: return "Created";
  };
}
}  // namespace osm
