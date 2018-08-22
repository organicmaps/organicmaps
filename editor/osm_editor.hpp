#pragma once

#include "editor/config_loader.hpp"
#include "editor/editor_config.hpp"
#include "editor/editor_notes.hpp"
#include "editor/editor_storage.hpp"
#include "editor/new_feature_categories.hpp"
#include "editor/xml_feature.hpp"

#include "indexer/editable_map_object.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/rect2d.hpp"

#include "base/atomic_shared_ptr.hpp"
#include "base/timer.hpp"

#include "std/ctime.hpp"
#include "std/function.hpp"
#include "std/map.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace editor
{
namespace testing
{
class EditorTest;
}
}
namespace editor
{
class XMLFeature;
}

namespace osm
{
// NOTE: this class is thead-safe for read operations,
// but write operations should be called on main thread only.
class Editor final : public MwmSet::Observer
{
  friend class editor::testing::EditorTest;

  Editor();

public:
  using FeatureTypeFn = function<void(FeatureType & ft)>;
  using InvalidateFn = function<void()>;
  using ForEachFeaturesNearByFn = function<void(FeatureTypeFn && fn, m2::PointD const & mercator)>;

  struct Delegate
  {
    virtual ~Delegate() = default;

    virtual MwmSet::MwmId GetMwmIdByMapName(string const & name) const = 0;
    virtual unique_ptr<FeatureType> GetOriginalFeature(FeatureID const & fid) const = 0;
    virtual string GetOriginalFeatureStreet(FeatureType & ft) const = 0;
    virtual void ForEachFeatureAtPoint(FeatureTypeFn && fn, m2::PointD const & point) const = 0;
  };

  enum class UploadResult
  {
    Success,
    Error,
    NothingToUpload
  };

  using FinishUploadCallback = function<void(UploadResult)>;

  enum class SaveResult
  {
    NothingWasChanged,
    SavedSuccessfully,
    NoFreeSpaceError,
    NoUnderlyingMapError,
    SavingError
  };

  enum class NoteProblemType
  {
    General,
    PlaceDoesNotExist
  };

  struct Stats
  {
    /// <id, feature status string>
    vector<pair<FeatureID, string>> m_edits;
    size_t m_uploadedCount = 0;
    time_t m_lastUploadTimestamp = my::INVALID_TIME_STAMP;
  };

  // Predefined messages.
  static const char * const kPlaceDoesNotExistMessage;

  static Editor & Instance();

  void SetDelegate(unique_ptr<Delegate> delegate) { m_delegate = move(delegate); }

  void SetStorageForTesting(unique_ptr<editor::StorageBase> storage) { m_storage = move(storage); }

  void ResetNotes() { m_notes = editor::Notes::MakeNotes(); }

  void SetDefaultStorage();

  void SetInvalidateFn(InvalidateFn const & fn) { m_invalidateFn = fn; }

  void LoadEdits();
  /// Resets editor to initial state: no any edits or created/deleted features.
  void ClearAllLocalEdits();

  void OnMapUpdated(platform::LocalCountryFile const &,
                    platform::LocalCountryFile const &) override
  {
    LoadEdits();
  }

  void OnMapDeregistered(platform::LocalCountryFile const & localFile) override;

  using FeatureIndexFunctor = function<void(uint32_t)>;
  void ForEachCreatedFeature(MwmSet::MwmId const & id, FeatureIndexFunctor const & f,
                             m2::RectD const & rect, int scale) const;

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

  /// Editor checks internally if any feature params were actually edited.
  SaveResult SaveEditedFeature(EditableMapObject const & emo);

  /// Removes changes from editor.
  /// @returns false if a feature was uploaded.
  bool RollBackChanges(FeatureID const & fid);

  EditableProperties GetEditableProperties(FeatureType & feature) const;

  bool HaveMapEditsOrNotesToUpload() const;
  bool HaveMapEditsToUpload(MwmSet::MwmId const & mwmId) const;

  using ChangesetTags = map<string, string>;
  /// Tries to upload all local changes to OSM server in a separate thread.
  /// @param[in] tags should provide additional information about client to use in changeset.
  void UploadChanges(string const & key, string const & secret, ChangesetTags tags,
                     FinishUploadCallback callBack = FinishUploadCallback());
  // TODO(mgsergio): Test new types from new config but with old classificator (where these types are absent).
  // Editor should silently ignore all types in config which are unknown to him.
  NewFeatureCategories GetNewFeatureCategories() const;

  bool CreatePoint(uint32_t type, m2::PointD const & mercator, MwmSet::MwmId const & id,
                   EditableMapObject & outFeature) const;

  void CreateNote(ms::LatLon const & latLon, FeatureID const & fid,
                  feature::TypesHolder const & holder, string const & defaultName,
                  NoteProblemType const type, string const & note);

  Stats GetStats() const;

  // Don't use this function to determine if a feature in editor was created.
  // Use GetFeatureStatus(fid) instead. This function is used when a feature is
  // not yet saved and we have to know if it was modified or created.
  static bool IsCreatedFeature(FeatureID const & fid);

private:
  // TODO(a): Use this structure as part of FeatureTypeInfo.
  struct UploadInfo
  {
    time_t m_uploadAttemptTimestamp = my::INVALID_TIME_STAMP;
    /// Is empty if upload has never occured or one of k* constants above otherwise.
    string m_uploadStatus;
    string m_uploadError;
  };

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

  using FeaturesContainer = map<MwmSet::MwmId, map<uint32_t, FeatureTypeInfo>>;

  /// @returns false if fails.
  bool Save(FeaturesContainer const & features) const;
  bool SaveTransaction(shared_ptr<FeaturesContainer> const & features);
  bool RemoveFeatureIfExists(FeatureID const & fid);
  /// Notify framework that something has changed and should be redisplayed.
  void Invalidate();

  // Saves a feature in internal storage with FeatureStatus::Obsolete status.
  bool MarkFeatureAsObsolete(FeatureID const & fid);
  bool RemoveFeature(FeatureID const & fid);

  FeatureID GenerateNewFeatureId(FeaturesContainer const & features,
                                 MwmSet::MwmId const & id) const;
  EditableProperties GetEditablePropertiesForTypes(feature::TypesHolder const & types) const;

  bool FillFeatureInfo(FeatureStatus status, editor::XMLFeature const & xml, FeatureID const & fid,
                       FeatureTypeInfo & fti) const;
  /// @returns pointer to m_features[id][index] if exists, nullptr otherwise.
  FeatureTypeInfo const * GetFeatureTypeInfo(FeaturesContainer const & features,
                                             MwmSet::MwmId const & mwmId, uint32_t index) const;
  void SaveUploadedInformation(FeatureID const & fid, UploadInfo const & fromUploader);

  void MarkFeatureWithStatus(FeaturesContainer & editableFeatures, FeatureID const & fid,
                             FeatureStatus status);

  // These methods are just checked wrappers around Delegate.
  MwmSet::MwmId GetMwmIdByMapName(string const & name);
  unique_ptr<FeatureType> GetOriginalFeature(FeatureID const & fid) const;
  string GetOriginalFeatureStreet(FeatureType & ft) const;
  void ForEachFeatureAtPoint(FeatureTypeFn && fn, m2::PointD const & point) const;
  FeatureID GetFeatureIdByXmlFeature(FeaturesContainer const & features,
                                     editor::XMLFeature const & xml, MwmSet::MwmId const & mwmId,
                                     FeatureStatus status, bool needMigrate) const;
  void LoadMwmEdits(FeaturesContainer & loadedFeatures, pugi::xml_node const & mwm,
                    MwmSet::MwmId const & mwmId, bool needMigrate);

  bool HaveMapEditsToUpload(FeaturesContainer const & features) const;

  FeatureStatus GetFeatureStatusImpl(FeaturesContainer const & features,
                                     MwmSet::MwmId const & mwmId, uint32_t index) const;

  bool IsFeatureUploadedImpl(FeaturesContainer const & features, MwmSet::MwmId const & mwmId,
                             uint32_t index) const;

  /// Deleted, edited and created features.
  base::AtomicSharedPtr<FeaturesContainer> m_features;

  unique_ptr<Delegate> m_delegate;

  /// Invalidate map viewport after edits.
  InvalidateFn m_invalidateFn;

  /// Contains information about what and how can be edited.
  base::AtomicSharedPtr<editor::EditorConfig> m_config;
  editor::ConfigLoader m_configLoader;

  /// Notes to be sent to osm.
  shared_ptr<editor::Notes> m_notes;

  unique_ptr<editor::StorageBase> m_storage;

  DECLARE_THREAD_CHECKER(MainThreadChecker);
};  // class Editor

string DebugPrint(Editor::SaveResult const saveResult);
}  // namespace osm
