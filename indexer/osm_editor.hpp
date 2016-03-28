#pragma once

#include "geometry/rect2d.hpp"

#include "indexer/editable_map_object.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/mwm_set.hpp"

#include "editor/editor_config.hpp"
#include "editor/editor_notes.hpp"
#include "editor/new_feature_categories.hpp"
#include "editor/xml_feature.hpp"

#include "base/timer.hpp"

#include "std/ctime.hpp"
#include "std/function.hpp"
#include "std/map.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace osm
{
class Editor final
{
  Editor();

public:
  using TFeatureTypeFn = function<void(FeatureType &)>;  // Mimics Framework::TFeatureTypeFn.

  using TMwmIdByMapNameFn = function<MwmSet::MwmId(string const & /*map*/)>;
  using TInvalidateFn = function<void()>;
  using TFeatureLoaderFn = function<unique_ptr<FeatureType> (FeatureID const & /*fid*/)>;
  using TFeatureOriginalStreetFn = function<string(FeatureType & /*ft*/)>;
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

  enum SaveResult
  {
    NothingWasChanged,
    SavedSuccessfully,
    NoFreeSpaceError
  };
  /// Editor checks internally if any feature params were actually edited.
  SaveResult SaveEditedFeature(EditableMapObject const & emo);

  EditableProperties GetEditableProperties(FeatureType const & feature) const;

  bool HaveMapEditsOrNotesToUpload() const;
  bool HaveMapEditsToUpload(MwmSet::MwmId const & mwmId) const;
  bool HaveMapEditsToUpload() const;
  using TChangesetTags = map<string, string>;
  /// Tries to upload all local changes to OSM server in a separate thread.
  /// @param[in] tags should provide additional information about client to use in changeset.
  void UploadChanges(string const & key, string const & secret, TChangesetTags tags,
                     TFinishUploadCallback callBack = TFinishUploadCallback());
  // TODO(mgsergio): Test new types from new config but with old classificator (where these types are absent).
  // Editor should silently ignore all types in config which are unknown to him.
  NewFeatureCategories GetNewFeatureCategories() const;

  bool CreatePoint(uint32_t type, m2::PointD const & mercator, MwmSet::MwmId const & id, EditableMapObject & outFeature);

  void CreateNote(m2::PointD const & point, string const & note);
  void UploadNotes(string const & key, string const & secret);

  struct Stats
  {
    /// <id, feature status string>
    vector<pair<FeatureID, string>> m_edits;
    size_t m_uploadedCount = 0;
    time_t m_lastUploadTimestamp = my::INVALID_TIME_STAMP;
  };
  Stats GetStats() const;
  
  static bool IsCreatedFeature(FeatureID const & fid);

private:
  // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
  /// @returns false if fails.
  bool Save(string const & fullFilePath) const;
  void RemoveFeatureFromStorageIfExists(MwmSet::MwmId const & mwmId, uint32_t index);
  /// Notify framework that something has changed and should be redisplayed.
  void Invalidate();

  FeatureID GenerateNewFeatureId(MwmSet::MwmId const & id);
  EditableProperties GetEditablePropertiesForTypes(feature::TypesHolder const & types) const;

  struct FeatureTypeInfo
  {
    FeatureStatus m_status;
    // TODO(AlexZ): Integrate EditableMapObject class into an editor instead of FeatureType.
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

  /// Contains information about what and how can be edited.
  editor::EditorConfig m_config;

  /// Notes to be sent to osm.
  shared_ptr<editor::Notes> m_notes;
};  // class Editor

string DebugPrint(Editor::FeatureStatus fs);

}  // namespace osm
