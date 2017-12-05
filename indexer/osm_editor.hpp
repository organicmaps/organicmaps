#pragma once

#include "geometry/rect2d.hpp"

#include "indexer/editable_map_object.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/new_feature_categories.hpp"

#include "editor/config_loader.hpp"
#include "editor/editor_config.hpp"
#include "editor/editor_notes.hpp"
#include "editor/editor_storage.hpp"
#include "editor/xml_feature.hpp"

#include "base/timer.hpp"

#include "std/ctime.hpp"
#include "std/function.hpp"
#include "std/map.hpp"
#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace editor
{
namespace testing
{
class EditorTest;
}  // namespace testing
}  // namespace editor

class Index;

namespace osm
{
class Editor final : public MwmSet::Observer
{
  friend class editor::testing::EditorTest;

  Editor();

public:
  using TFeatureTypeFn = function<void(FeatureType &)>;  // Mimics Framework::TFeatureTypeFn.

  using TMwmIdByMapNameFn = function<MwmSet::MwmId(string const & /*map*/)>;
  using TInvalidateFn = function<void()>;
  using TFeatureLoaderFn = function<unique_ptr<FeatureType> (FeatureID const & /*fid*/)>;
  using TFeatureOriginalStreetFn = function<string(FeatureType & /*ft*/)>;
  using TForEachFeaturesNearByFn =
      function<void(TFeatureTypeFn && /*fn*/, m2::PointD const & /*mercator*/)>;

  struct Delegate
  {
    virtual ~Delegate() = default;

    virtual MwmSet::MwmId GetMwmIdByMapName(string const & name) const = 0;
    virtual unique_ptr<FeatureType> GetOriginalFeature(FeatureID const & fid) const = 0;
    virtual string GetOriginalFeatureStreet(FeatureType & ft) const = 0;
    virtual void ForEachFeatureAtPoint(TFeatureTypeFn && fn, m2::PointD const & point) const = 0;
  };

  enum class UploadResult
  {
    Success,
    Error,
    NothingToUpload
  };
  using TFinishUploadCallback = function<void(UploadResult)>;

  enum class FeatureStatus
  {
    Untouched,  // The feature hasn't been saved in the editor.
    Deleted,    // The feature has been marked as deleted.
    Obsolete,   // The feature has been marked for deletion via note.
    Modified,   // The feature has been saved in the editor and differs from the original one.
    Created     // The feature was created by a user and has been saved in the editor.
                // Note: If a feature was created by a user but hasn't been saved in the editor yet
                // its status is Untouched.
  };

  static Editor & Instance();

  inline void SetDelegate(unique_ptr<Delegate> delegate) { m_delegate = move(delegate); }

  inline void SetStorageForTesting(unique_ptr<editor::StorageBase> storage)
  {
    m_storage = move(storage);
  }

  void SetDefaultStorage();

  void SetInvalidateFn(TInvalidateFn const & fn) { m_invalidateFn = fn; }

  void LoadMapEdits();
  /// Resets editor to initial state: no any edits or created/deleted features.
  void ClearAllLocalEdits();

  void OnMapUpdated(platform::LocalCountryFile const &,
                    platform::LocalCountryFile const &) override
  {
    LoadMapEdits();
  }

  void OnMapDeregistered(platform::LocalCountryFile const & localFile) override;

  using TFeatureIDFunctor = function<void(FeatureID const &)>;
  void ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                       TFeatureIDFunctor const & f,
                                       m2::RectD const & rect,
                                       int scale);
  using TFeatureTypeFunctor = function<void(FeatureType &)>;
  void ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                       TFeatureTypeFunctor const & f,
                                       m2::RectD const & rect,
                                       int scale);

  // TODO(mgsergio): Unify feature functions signatures.

  /// Easy way to check if a feature was deleted, modified, created or not changed at all.
  FeatureStatus GetFeatureStatus(MwmSet::MwmId const & mwmId, uint32_t index) const;
  FeatureStatus GetFeatureStatus(FeatureID const & fid) const;

  /// @returns true if a feature was uploaded to osm.
  bool IsFeatureUploaded(MwmSet::MwmId const & mwmId, uint32_t index) const;

  /// Marks feature as "deleted" from MwM file.
  void DeleteFeature(FeatureID const & fid);

  /// @returns false if feature wasn't edited.
  /// @param outFeature is valid only if true was returned.
  bool GetEditedFeature(MwmSet::MwmId const & mwmId, uint32_t index, FeatureType & outFeature) const;
  bool GetEditedFeature(FeatureID const & fid, FeatureType & outFeature) const;

  /// @returns false if feature wasn't edited.
  /// @param outFeatureStreet is valid only if true was returned.
  bool GetEditedFeatureStreet(FeatureID const & fid, string & outFeatureStreet) const;

  /// @returns sorted features indices with specified status.
  vector<uint32_t> GetFeaturesByStatus(MwmSet::MwmId const & mwmId, FeatureStatus status) const;

  enum SaveResult
  {
    NothingWasChanged,
    SavedSuccessfully,
    NoFreeSpaceError,
    NoUnderlyingMapError,
    SavingError
  };
  /// Editor checks internally if any feature params were actually edited.
  SaveResult SaveEditedFeature(EditableMapObject const & emo);

  /// Removes changes from editor.
  /// @returns false if a feature was uploaded.
  bool RollBackChanges(FeatureID const & fid);

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

  bool CreatePoint(uint32_t type, m2::PointD const & mercator,
                   MwmSet::MwmId const & id, EditableMapObject & outFeature);

  // Predefined messages.
  static const char * const kPlaceDoesNotExistMessage;

  enum class NoteProblemType
  {
    General,
    PlaceDoesNotExist
  };

  void CreateNote(ms::LatLon const & latLon, FeatureID const & fid,
                  feature::TypesHolder const & holder, string const & defaultName,
                  NoteProblemType const type, string const & note);
  void UploadNotes(string const & key, string const & secret);

  struct Stats
  {
    /// <id, feature status string>
    vector<pair<FeatureID, string>> m_edits;
    size_t m_uploadedCount = 0;
    time_t m_lastUploadTimestamp = my::INVALID_TIME_STAMP;
  };
  Stats GetStats() const;

  // Don't use this function to determine if a feature in editor was created.
  // Use GetFeatureStatus(fid) instead. This function is used when a feature is
  // not yet saved and we have to know if it was modified or created.
  static bool IsCreatedFeature(FeatureID const & fid);
  // Returns true if the original feature has default name.
  bool OriginalFeatureHasDefaultName(FeatureID const & fid) const;

private:
  // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
  /// @returns false if fails.
  bool Save() const;
  void RemoveFeatureFromStorageIfExists(MwmSet::MwmId const & mwmId, uint32_t index);
  void RemoveFeatureFromStorageIfExists(FeatureID const & fid);
  /// Notify framework that something has changed and should be redisplayed.
  void Invalidate();

  // Saves a feature in internal storage with FeatureStatus::Obsolete status.
  bool MarkFeatureAsObsolete(FeatureID const & fid);
  bool RemoveFeature(FeatureID const & fid);

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
  /// @returns pointer to m_features[id][index] if exists, nullptr otherwise.
  FeatureTypeInfo const * GetFeatureTypeInfo(MwmSet::MwmId const & mwmId, uint32_t index) const;
  FeatureTypeInfo * GetFeatureTypeInfo(MwmSet::MwmId const & mwmId, uint32_t index);
  void SaveUploadedInformation(FeatureTypeInfo const & fromUploader);

  void MarkFeatureWithStatus(FeatureID const & fid, FeatureStatus status);

  // These methods are just checked wrappers around Delegate.
  MwmSet::MwmId GetMwmIdByMapName(string const & name);
  unique_ptr<FeatureType> GetOriginalFeature(FeatureID const & fid) const;
  string GetOriginalFeatureStreet(FeatureType & ft) const;
  void ForEachFeatureAtPoint(TFeatureTypeFn && fn, m2::PointD const & point) const;

  // TODO(AlexZ): Synchronize multithread access.
  /// Deleted, edited and created features.
  map<MwmSet::MwmId, map<uint32_t, FeatureTypeInfo>> m_features;

  unique_ptr<Delegate> m_delegate;

  /// Invalidate map viewport after edits.
  TInvalidateFn m_invalidateFn;

  /// Contains information about what and how can be edited.
  editor::EditorConfigWrapper m_config;
  editor::ConfigLoader m_configLoader;

  /// Notes to be sent to osm.
  shared_ptr<editor::Notes> m_notes;
  // Mutex which locks OnMapDeregistered method
  mutex m_mapDeregisteredMutex;

  unique_ptr<editor::StorageBase> m_storage;
};  // class Editor

string DebugPrint(Editor::FeatureStatus fs);

}  // namespace osm
