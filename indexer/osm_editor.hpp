#pragma once

#include "geometry/rect2d.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/mwm_set.hpp"

#include "editor/xml_feature.hpp"

#include "base/timer.hpp"

#include "std/ctime.hpp"
#include "std/function.hpp"
#include "std/map.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace osm
{
/// Holds information to construct editor's UI.
struct EditableProperties
{
  bool m_name = false;
  bool m_address = false;
  vector<feature::Metadata::EType> m_metadata;
  bool IsEditable() const { return m_name || m_address || !m_metadata.empty(); }
};

class Editor final
{
  Editor() = default;

public:
  using TFeatureTypeFn = function<void(FeatureType &)>;  // Mimics Framework::TFeatureTypeFn.

  using TMwmIdByMapNameFn = function<MwmSet::MwmId(string const & /*map*/)>;
  using TInvalidateFn = function<void()>;
  using TFeatureLoaderFn = function<unique_ptr<FeatureType> (FeatureID const & /*fid*/)>;
  using TFeatureOriginalStreetFn = function<string(FeatureType const & /*ft*/)>;
  using TForEachFeaturesNearByFn =
      function<void(TFeatureTypeFn && /*fn*/, m2::PointD const & /*mercator*/)>;
  enum class UploadResult
  {
    Success,
    Error,
    NothingToUpload
  };
  using TFinishUploadCallback = function<void(UploadResult)>;

  enum class FeatureStatus
  {
    Untouched,
    Deleted,
    Modified,
    Created
  };

  static Editor & Instance();

  void SetMwmIdByNameAndVersionFn(TMwmIdByMapNameFn const & fn) { m_mwmIdByMapNameFn = fn; }
  void SetInvalidateFn(TInvalidateFn const & fn) { m_invalidateFn = fn; }
  void SetFeatureLoaderFn(TFeatureLoaderFn const & fn) { m_getOriginalFeatureFn = fn; }
  void SetFeatureOriginalStreetFn(TFeatureOriginalStreetFn const & fn) { m_getOriginalFeatureStreetFn = fn; }
  void SetForEachFeatureAtPointFn(TForEachFeaturesNearByFn const & fn) { m_forEachFeatureAtPointFn = fn; }

  void LoadMapEdits();
  /// Resets editor to initial state: no any edits or created/deleted features.
  void ClearAllLocalEdits();

  using TFeatureIDFunctor = function<void(FeatureID const &)>;
  void ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                       TFeatureIDFunctor const & f,
                                       m2::RectD const & rect,
                                       uint32_t scale);
  using TFeatureTypeFunctor = function<void(FeatureType &)>;
  void ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                       TFeatureTypeFunctor const & f,
                                       m2::RectD const & rect,
                                       uint32_t scale);

  /// Easy way to check if feature was deleted, modified, created or not changed at all.
  FeatureStatus GetFeatureStatus(MwmSet::MwmId const & mwmId, uint32_t index) const;

  /// Marks feature as "deleted" from MwM file.
  void DeleteFeature(FeatureType const & feature);

  /// @returns false if feature wasn't edited.
  /// @param outFeature is valid only if true was returned.
  bool GetEditedFeature(MwmSet::MwmId const & mwmId, uint32_t index, FeatureType & outFeature) const;

  /// @returns false if feature wasn't edited.
  /// @param outFeatureStreet is valid only if true was returned.
  bool GetEditedFeatureStreet(FeatureID const & fid, string & outFeatureStreet) const;

  /// @returns sorted features indices with specified status.
  vector<uint32_t> GetFeaturesByStatus(MwmSet::MwmId const & mwmId, FeatureStatus status) const;

  /// Editor checks internally if any feature params were actually edited.
  /// House number is correctly updated for editedFeature (if it's valid).
  void EditFeature(FeatureType & editedFeature,
                   string const & editedStreet = "",
                   string const & editedHouseNumber = "");

  EditableProperties GetEditableProperties(FeatureType const & feature) const;

  bool HaveSomethingToUpload() const;
  using TChangesetTags = map<string, string>;
  /// Tries to upload all local changes to OSM server in a separate thread.
  /// @param[in] tags should provide additional information about client to use in changeset.
  void UploadChanges(string const & key, string const & secret, TChangesetTags tags,
                     TFinishUploadCallback callBack = TFinishUploadCallback());

  struct Stats
  {
    /// <id, feature status string>
    vector<pair<FeatureID, string>> m_edits;
    size_t m_uploadedCount = 0;
    time_t m_lastUploadTimestamp = my::INVALID_TIME_STAMP;
  };
  Stats GetStats() const;

private:
  // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
  void Save(string const & fullFilePath) const;
  void RemoveFeatureFromStorageIfExists(MwmSet::MwmId const & mwmId, uint32_t index);
  /// Notify framework that something has changed and should be redisplayed.
  void Invalidate();

  struct FeatureTypeInfo
  {
    FeatureStatus m_status;
    FeatureType m_feature;
    /// If not empty contains Feature's addr:street, edited by user.
    string m_street;
    time_t m_modificationTimestamp = my::INVALID_TIME_STAMP;
    time_t m_uploadAttemptTimestamp = my::INVALID_TIME_STAMP;
    /// Is empty if upload has never occured or one of k* constants above otherwise.
    string m_uploadStatus;
    string m_uploadError;
  };
  void SaveUploadedInformation(FeatureTypeInfo const & fromUploader);

  // TODO(AlexZ): Synchronize multithread access.
  /// Deleted, edited and created features.
  map<MwmSet::MwmId, map<uint32_t, FeatureTypeInfo>> m_features;

  /// Get MwmId for each map, used in FeatureIDs and to check if edits are up-to-date.
  TMwmIdByMapNameFn m_mwmIdByMapNameFn;
  /// Invalidate map viewport after edits.
  TInvalidateFn m_invalidateFn;
  /// Get FeatureType from mwm.
  TFeatureLoaderFn m_getOriginalFeatureFn;
  /// Get feature original street name or empty string.
  TFeatureOriginalStreetFn m_getOriginalFeatureStreetFn;
  /// Iterate over all features in some area that includes given point.
  TForEachFeaturesNearByFn m_forEachFeatureAtPointFn;
};  // class Editor

string DebugPrint(Editor::FeatureStatus fs);

}  // namespace osm
