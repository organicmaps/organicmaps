#pragma once

#include "map/bookmark.hpp"
#include "map/bookmark_catalog.hpp"
#include "map/bookmark_helpers.hpp"
#include "map/cloud.hpp"
#include "map/track.hpp"
#include "map/user_mark_layer.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "platform/safe_callback.hpp"

#include "base/macros.hpp"
#include "base/strings_bundle.hpp"
#include "base/thread_checker.hpp"

#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/optional.hpp>

class User;

class BookmarkManager final
{
  using UserMarkLayers = std::vector<std::unique_ptr<UserMarkLayer>>;
  using CategoriesCollection = std::map<kml::MarkGroupId, std::unique_ptr<BookmarkCategory>>;

  using MarksCollection = std::map<kml::MarkId, std::unique_ptr<UserMark>>;
  using BookmarksCollection = std::map<kml::MarkId, std::unique_ptr<Bookmark>>;
  using TracksCollection = std::map<kml::TrackId, std::unique_ptr<Track>>;

public:
  using KMLDataCollection = std::vector<std::pair<std::string, std::unique_ptr<kml::FileData>>>;
  using KMLDataCollectionPtr = std::shared_ptr<KMLDataCollection>;

  using AsyncLoadingStartedCallback = std::function<void()>;
  using AsyncLoadingFinishedCallback = std::function<void()>;
  using AsyncLoadingFileCallback = std::function<void(std::string const &, bool)>;

  struct AsyncLoadingCallbacks
  {
    AsyncLoadingStartedCallback m_onStarted;
    AsyncLoadingFinishedCallback m_onFinished;
    AsyncLoadingFileCallback m_onFileError;
    AsyncLoadingFileCallback m_onFileSuccess;
  };

  struct Callbacks
  {
    using GetStringsBundleFn = std::function<StringsBundle const &()>;
    using CreatedBookmarksCallback = std::function<void(std::vector<std::pair<kml::MarkId, kml::BookmarkData>> const &)>;
    using UpdatedBookmarksCallback = std::function<void(std::vector<std::pair<kml::MarkId, kml::BookmarkData>> const &)>;
    using DeletedBookmarksCallback = std::function<void(std::vector<kml::MarkId> const &)>;

    template <typename StringsBundleGetter, typename CreateListener, typename UpdateListener, typename DeleteListener>
    Callbacks(StringsBundleGetter && stringsBundleGetter, CreateListener && createListener,
              UpdateListener && updateListener, DeleteListener && deleteListener)
        : m_getStringsBundle(std::forward<StringsBundleGetter>(stringsBundleGetter))
        , m_createdBookmarksCallback(std::forward<CreateListener>(createListener))
        , m_updatedBookmarksCallback(std::forward<UpdateListener>(updateListener))
        , m_deletedBookmarksCallback(std::forward<DeleteListener>(deleteListener))
    {}

    GetStringsBundleFn m_getStringsBundle;
    CreatedBookmarksCallback m_createdBookmarksCallback;
    UpdatedBookmarksCallback m_updatedBookmarksCallback;
    DeletedBookmarksCallback m_deletedBookmarksCallback;
  };

  class EditSession
  {
  public:
    explicit EditSession(BookmarkManager & bmManager);
    ~EditSession();

    template <typename UserMarkT>
    UserMarkT * CreateUserMark(m2::PointD const & ptOrg)
    {
      return m_bmManager.CreateUserMark<UserMarkT>(ptOrg);
    }

    Bookmark * CreateBookmark(kml::BookmarkData && bmData);
    Bookmark * CreateBookmark(kml::BookmarkData && bmData, kml::MarkGroupId groupId);
    Track * CreateTrack(kml::TrackData && trackData);

    template <typename UserMarkT>
    UserMarkT * GetMarkForEdit(kml::MarkId markId)
    {
      return m_bmManager.GetMarkForEdit<UserMarkT>(markId);
    }

    Bookmark * GetBookmarkForEdit(kml::MarkId bmId);

    template <typename UserMarkT, typename F>
    void DeleteUserMarks(UserMark::Type type, F && deletePredicate)
    {
      return m_bmManager.DeleteUserMarks<UserMarkT>(type, std::move(deletePredicate));
    };

    void DeleteUserMark(kml::MarkId markId);
    void DeleteBookmark(kml::MarkId bmId);
    void DeleteTrack(kml::TrackId trackId);

    void ClearGroup(kml::MarkGroupId groupId);

    void SetIsVisible(kml::MarkGroupId groupId, bool visible);

    void MoveBookmark(kml::MarkId bmID, kml::MarkGroupId curGroupID, kml::MarkGroupId newGroupID);
    void UpdateBookmark(kml::MarkId bmId, kml::BookmarkData const & bm);

    void AttachBookmark(kml::MarkId bmId, kml::MarkGroupId groupId);
    void DetachBookmark(kml::MarkId bmId, kml::MarkGroupId groupId);

    void AttachTrack(kml::TrackId trackId, kml::MarkGroupId groupId);
    void DetachTrack(kml::TrackId trackId, kml::MarkGroupId groupId);

    void SetCategoryName(kml::MarkGroupId categoryId, std::string const & name);
    void SetCategoryDescription(kml::MarkGroupId categoryId, std::string const & desc);
    void SetCategoryTags(kml::MarkGroupId categoryId, std::vector<std::string> const & tags);
    void SetCategoryAccessRules(kml::MarkGroupId categoryId, kml::AccessRules accessRules);
    void SetCategoryCustomProperty(kml::MarkGroupId categoryId, std::string const & key,
                                   std::string const & value);
    bool DeleteBmCategory(kml::MarkGroupId groupId);

    void NotifyChanges();

  private:
    BookmarkManager & m_bmManager;
  };

  BookmarkManager(User & user, Callbacks && callbacks);

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

  void SetAsyncLoadingCallbacks(AsyncLoadingCallbacks && callbacks);
  bool IsAsyncLoadingInProgress() const { return m_asyncLoadingInProgress; }

  EditSession GetEditSession();

  void UpdateViewport(ScreenBase const & screen);
  void Teardown();

  static bool IsBookmarkCategory(kml::MarkGroupId groupId) { return groupId >= UserMark::USER_MARK_TYPES_COUNT_MAX; }
  static bool IsBookmark(kml::MarkId markId) { return UserMark::GetMarkType(markId) == UserMark::BOOKMARK; }
  static UserMark::Type GetGroupType(kml::MarkGroupId groupId)
  {
    return IsBookmarkCategory(groupId) ? UserMark::BOOKMARK : static_cast<UserMark::Type>(groupId);
  }

  template <typename UserMarkT>
  UserMarkT const * GetMark(kml::MarkId markId) const
  {
    auto * mark = GetUserMark(markId);
    ASSERT(dynamic_cast<UserMarkT const *>(mark) != nullptr, ());
    return static_cast<UserMarkT const *>(mark);
  }

  UserMark const * GetUserMark(kml::MarkId markId) const;
  Bookmark const * GetBookmark(kml::MarkId markId) const;
  Track const * GetTrack(kml::TrackId trackId) const;

  kml::MarkIdSet const & GetUserMarkIds(kml::MarkGroupId groupId) const;
  kml::TrackIdSet const & GetTrackIds(kml::MarkGroupId groupId) const;

  bool IsVisible(kml::MarkGroupId groupId) const;

  kml::MarkGroupId CreateBookmarkCategory(kml::CategoryData && data, bool autoSave = true);
  kml::MarkGroupId CreateBookmarkCategory(std::string const & name, bool autoSave = true);

  std::string GetCategoryName(kml::MarkGroupId categoryId) const;
  std::string GetCategoryFileName(kml::MarkGroupId categoryId) const;
  kml::CategoryData const & GetCategoryData(kml::MarkGroupId categoryId) const;

  kml::MarkGroupId GetCategoryId(std::string const & name) const;

  kml::GroupIdCollection const & GetBmGroupsIdList() const { return m_bmGroupsIdList; }
  bool HasBmCategory(kml::MarkGroupId groupId) const;
  kml::MarkGroupId LastEditedBMCategory();
  kml::PredefinedColor LastEditedBMColor() const;

  void SetLastEditedBmCategory(kml::MarkGroupId groupId);
  void SetLastEditedBmColor(kml::PredefinedColor color);

  using TTouchRectHolder = function<m2::AnyRectD(UserMark::Type)>;
  UserMark const * FindNearestUserMark(TTouchRectHolder const & holder) const;
  UserMark const * FindNearestUserMark(m2::AnyRectD const & rect) const;
  UserMark const * FindMarkInRect(kml::MarkGroupId groupId, m2::AnyRectD const & rect, double & d) const;

  /// Scans and loads all kml files with bookmarks.
  void LoadBookmarks();
  void LoadBookmark(std::string const & filePath, bool isTemporaryFile);

  /// Uses the same file name from which was loaded, or
  /// creates unique file name on first save and uses it every time.
  void SaveBookmarks(kml::GroupIdCollection const & groupIdCollection);

  StaticMarkPoint & SelectionMark() { return *m_selectionMark; }
  StaticMarkPoint const & SelectionMark() const { return *m_selectionMark; }

  MyPositionMarkPoint & MyPositionMark() { return *m_myPositionMark; }
  MyPositionMarkPoint const & MyPositionMark() const { return *m_myPositionMark; }

  void SetCloudEnabled(bool enabled);
  bool IsCloudEnabled() const;
  uint64_t GetLastSynchronizationTimestampInMs() const;
  std::unique_ptr<User::Subscriber> GetUserSubscriber();

  enum class CategoryFilterType
  {
    Private = 0,
    Public,
    All
  };

  struct SharingResult
  {
    enum class Code
    {
      Success = 0,
      EmptyCategory,
      ArchiveError,
      FileError
    };
    kml::MarkGroupId m_categoryId;
    Code m_code;
    std::string m_sharingPath;
    std::string m_errorString;

    SharingResult(kml::MarkGroupId categoryId, std::string const & sharingPath)
      : m_categoryId(categoryId)
      , m_code(Code::Success)
      , m_sharingPath(sharingPath)
    {}

    SharingResult(kml::MarkGroupId categoryId, Code code)
      : m_categoryId(categoryId)
      , m_code(code)
    {}

    SharingResult(kml::MarkGroupId categoryId, Code code, std::string const & errorString)
      : m_categoryId(categoryId)
      , m_code(code)
      , m_errorString(errorString)
    {}
  };

  using SharingHandler = platform::SafeCallback<void(SharingResult const & result)>;
  void PrepareFileForSharing(kml::MarkGroupId categoryId, SharingHandler && handler);

  bool IsCategoryEmpty(kml::MarkGroupId categoryId) const;

  bool IsEditableBookmark(kml::MarkId bmId) const;
  bool IsEditableTrack(kml::TrackId trackId) const;
  bool IsEditableCategory(kml::MarkGroupId groupId) const;

  bool IsUsedCategoryName(std::string const & name) const;
  bool AreAllCategoriesVisible(CategoryFilterType const filter) const;
  bool AreAllCategoriesInvisible(CategoryFilterType const filter) const;
  void SetAllCategoriesVisibility(CategoryFilterType const filter, bool visible);

  // Return number of files for the conversion to the binary format.
  size_t GetKmlFilesCountForConversion() const;

  // Convert all found kml files to the binary format.
  using ConversionHandler = platform::SafeCallback<void(bool success)>;
  void ConvertAllKmlFiles(ConversionHandler && handler);

  // These handlers are always called from UI-thread.
  void SetCloudHandlers(Cloud::SynchronizationStartedHandler && onSynchronizationStarted,
                        Cloud::SynchronizationFinishedHandler && onSynchronizationFinished,
                        Cloud::RestoreRequestedHandler && onRestoreRequested,
                        Cloud::RestoredFilesPreparedHandler && onRestoredFilesPrepared);

  void RequestCloudRestoring();
  void ApplyCloudRestoring();
  void CancelCloudRestoring();

  void SetNotificationsEnabled(bool enabled);
  bool AreNotificationsEnabled() const;

  using OnCatalogDownloadStartedHandler = platform::SafeCallback<void(std::string const & id)>;
  using OnCatalogDownloadFinishedHandler = platform::SafeCallback<void(std::string const & id,
                                                                       BookmarkCatalog::DownloadResult result)>;
  using OnCatalogImportStartedHandler = platform::SafeCallback<void(std::string const & id)>;
  using OnCatalogImportFinishedHandler = platform::SafeCallback<void(std::string const & id,
                                                                     kml::MarkGroupId categoryId,
                                                                     bool successful)>;
  using OnCatalogUploadStartedHandler = std::function<void(kml::MarkGroupId originCategoryId)>;
  using OnCatalogUploadFinishedHandler = std::function<void(BookmarkCatalog::UploadResult uploadResult,
                                                            std::string const & description,
                                                            kml::MarkGroupId originCategoryId,
                                                            kml::MarkGroupId resultCategoryId)>;

  void SetCatalogHandlers(OnCatalogDownloadStartedHandler && onCatalogDownloadStarted,
                          OnCatalogDownloadFinishedHandler && onCatalogDownloadFinished,
                          OnCatalogImportStartedHandler && onCatalogImportStarted,
                          OnCatalogImportFinishedHandler && onCatalogImportFinished,
                          OnCatalogUploadStartedHandler && onCatalogUploadStartedHandler,
                          OnCatalogUploadFinishedHandler && onCatalogUploadFinishedHandler);
  void DownloadFromCatalogAndImport(std::string const & id, std::string const & name);
  void ImportDownloadedFromCatalog(std::string const & id, std::string const & filePath);
  void UploadToCatalog(kml::MarkGroupId categoryId, kml::AccessRules accessRules);
  bool IsCategoryFromCatalog(kml::MarkGroupId categoryId) const;
  std::string GetCategoryCatalogDeeplink(kml::MarkGroupId categoryId) const;
  BookmarkCatalog const & GetCatalog() const;

  bool IsMyCategory(kml::MarkGroupId categoryId) const;

  /// These functions are public for unit tests only. You shouldn't call them from client code.
  void EnableTestMode(bool enable);
  bool SaveBookmarkCategory(kml::MarkGroupId groupId);
  bool SaveBookmarkCategory(kml::MarkGroupId groupId, Writer & writer, KmlFileType fileType) const;
  void CreateCategories(KMLDataCollection && dataCollection, bool autoSave = true);
  static std::string RemoveInvalidSymbols(std::string const & name, std::string const & defaultName);
  static std::string GenerateUniqueFileName(std::string const & path, std::string name, std::string const & fileExt);
  static std::string GenerateValidAndUniqueFilePathForKML(std::string const & fileName);
  static std::string GenerateValidAndUniqueFilePathForKMB(std::string const & fileName);
  static std::string GetActualBookmarksDirectory();
  static bool IsMigrated();

private:
  class MarksChangesTracker : public df::UserMarksProvider
  {
  public:
    explicit MarksChangesTracker(BookmarkManager & bmManager) : m_bmManager(bmManager) {}

    void OnAddMark(kml::MarkId markId);
    void OnDeleteMark(kml::MarkId markId);
    void OnUpdateMark(kml::MarkId markId);

    void OnAddLine(kml::TrackId lineId);
    void OnDeleteLine(kml::TrackId lineId);

    void OnAddGroup(kml::MarkGroupId groupId);
    void OnDeleteGroup(kml::MarkGroupId groupId);

    bool CheckChanges();
    void ResetChanges();

    // UserMarksProvider
    kml::GroupIdSet GetAllGroupIds() const override;
    kml::GroupIdSet const & GetDirtyGroupIds() const override { return m_dirtyGroups; }
    kml::GroupIdSet const & GetRemovedGroupIds() const override { return m_removedGroups; }
    kml::MarkIdSet const & GetCreatedMarkIds() const override { return m_createdMarks; }
    kml::MarkIdSet const & GetRemovedMarkIds() const override { return m_removedMarks; }
    kml::MarkIdSet const & GetUpdatedMarkIds() const override { return m_updatedMarks; }
    kml::TrackIdSet const & GetRemovedLineIds() const override { return m_removedLines; }
    bool IsGroupVisible(kml::MarkGroupId groupId) const override;
    bool IsGroupVisibilityChanged(kml::MarkGroupId groupId) const override;
    kml::MarkIdSet const & GetGroupPointIds(kml::MarkGroupId groupId) const override;
    kml::TrackIdSet const & GetGroupLineIds(kml::MarkGroupId groupId) const override;
    df::UserPointMark const * GetUserPointMark(kml::MarkId markId) const override;
    df::UserLineMark const * GetUserLineMark(kml::TrackId lineId) const override;

  private:
    BookmarkManager & m_bmManager;

    kml::MarkIdSet m_createdMarks;
    kml::MarkIdSet m_removedMarks;
    kml::MarkIdSet m_updatedMarks;

    kml::TrackIdSet m_createdLines;
    kml::TrackIdSet m_removedLines;

    kml::GroupIdSet m_dirtyGroups;
    kml::GroupIdSet m_createdGroups;
    kml::GroupIdSet m_removedGroups;
  };

  template <typename UserMarkT>
  UserMarkT * CreateUserMark(m2::PointD const & ptOrg)
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());
    auto mark = std::make_unique<UserMarkT>(ptOrg);
    auto * m = mark.get();
    auto const markId = m->GetId();
    auto const groupId = static_cast<kml::MarkGroupId>(m->GetMarkType());
    CHECK_EQUAL(m_userMarks.count(markId), 0, ());
    ASSERT_GREATER(groupId, 0, ());
    ASSERT_LESS(groupId - 1, m_userMarkLayers.size(), ());
    m_userMarks.emplace(markId, std::move(mark));
    m_changesTracker.OnAddMark(markId);
    m_userMarkLayers[groupId - 1]->AttachUserMark(markId);
    return m;
  }

  template <typename UserMarkT>
  UserMarkT * GetMarkForEdit(kml::MarkId markId)
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());
    auto * mark = GetUserMarkForEdit(markId);
    ASSERT(dynamic_cast<UserMarkT *>(mark) != nullptr, ());
    return static_cast<UserMarkT *>(mark);
  }

  template <typename UserMarkT, typename F>
  void DeleteUserMarks(UserMark::Type type, F && deletePredicate)
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());
    std::list<kml::MarkId> marksToDelete;
    for (auto markId : GetUserMarkIds(type))
    {
      if (deletePredicate(GetMark<UserMarkT>(markId)))
        marksToDelete.push_back(markId);
    }
    // Delete after iterating to avoid iterators invalidation issues.
    for (auto markId : marksToDelete)
      DeleteUserMark(markId);
  };

  UserMark * GetUserMarkForEdit(kml::MarkId markId);
  void DeleteUserMark(kml::MarkId markId);

  Bookmark * CreateBookmark(kml::BookmarkData && bmData);
  Bookmark * CreateBookmark(kml::BookmarkData && bmData, kml::MarkGroupId groupId);

  Bookmark * GetBookmarkForEdit(kml::MarkId markId);
  void AttachBookmark(kml::MarkId bmId, kml::MarkGroupId groupId);
  void DetachBookmark(kml::MarkId bmId, kml::MarkGroupId groupId);
  void DeleteBookmark(kml::MarkId bmId);

  Track * CreateTrack(kml::TrackData && trackData);

  void AttachTrack(kml::TrackId trackId, kml::MarkGroupId groupId);
  void DetachTrack(kml::TrackId trackId, kml::MarkGroupId groupId);
  void DeleteTrack(kml::TrackId trackId);

  void ClearGroup(kml::MarkGroupId groupId);
  void SetIsVisible(kml::MarkGroupId groupId, bool visible);

  void SetCategoryName(kml::MarkGroupId categoryId, std::string const & name);
  void SetCategoryDescription(kml::MarkGroupId categoryId, std::string const & desc);
  void SetCategoryTags(kml::MarkGroupId categoryId, std::vector<std::string> const & tags);
  void SetCategoryAccessRules(kml::MarkGroupId categoryId, kml::AccessRules accessRules);
  void SetCategoryCustomProperty(kml::MarkGroupId categoryId, std::string const & key,
                                 std::string const & value);
  bool DeleteBmCategory(kml::MarkGroupId groupId);
  void ClearCategories();

  void MoveBookmark(kml::MarkId bmID, kml::MarkGroupId curGroupID, kml::MarkGroupId newGroupID);
  void UpdateBookmark(kml::MarkId bmId, kml::BookmarkData const & bm);

  UserMark const * GetMark(kml::MarkId markId) const;

  UserMarkLayer const * GetGroup(kml::MarkGroupId groupId) const;
  UserMarkLayer * GetGroup(kml::MarkGroupId groupId);
  BookmarkCategory const * GetBmCategory(kml::MarkGroupId categoryId) const;
  BookmarkCategory * GetBmCategory(kml::MarkGroupId categoryId);

  Bookmark * AddBookmark(std::unique_ptr<Bookmark> && bookmark);
  Track * AddTrack(std::unique_ptr<Track> && track);

  void OnEditSessionOpened();
  void OnEditSessionClosed();
  void NotifyChanges();

  void SaveState() const;
  void LoadState();
  void NotifyAboutStartAsyncLoading();
  void NotifyAboutFinishAsyncLoading(KMLDataCollectionPtr && collection);
  boost::optional<std::string> GetKMLPath(std::string const & filePath);
  void NotifyAboutFile(bool success, std::string const & filePath, bool isTemporaryFile);
  void LoadBookmarkRoutine(std::string const & filePath, bool isTemporaryFile);
  
  using BookmarksChecker = std::function<bool(kml::FileData const &)>;
  KMLDataCollectionPtr LoadBookmarks(std::string const & dir, std::string const & ext,
                                     KmlFileType fileType, BookmarksChecker const & checker,
                                     std::vector<std::string> & cloudFilePaths);

  void CollectDirtyGroups(kml::GroupIdSet & dirtyGroups);

  void UpdateBmGroupIdList();

  void SendBookmarksChanges();
  void GetBookmarksData(kml::MarkIdSet const & markIds,
                        std::vector<std::pair<kml::MarkId, kml::BookmarkData>> & data) const;
  kml::MarkGroupId CheckAndCreateDefaultCategory();
  void CheckAndResetLastIds();

  std::unique_ptr<kml::FileData> CollectBmGroupKMLData(BookmarkCategory const * group) const;
  KMLDataCollectionPtr PrepareToSaveBookmarks(kml::GroupIdCollection const & groupIdCollection);
  bool SaveKmlFileSafe(kml::FileData & kmlData, std::string const & file);

  void OnSynchronizationStarted(Cloud::SynchronizationType type);
  void OnSynchronizationFinished(Cloud::SynchronizationType type, Cloud::SynchronizationResult result,
                                 std::string const & errorStr);
  void OnRestoreRequested(Cloud::RestoringRequestResult result, std::string const & deviceName,
                          uint64_t backupTimestampInMs);
  void OnRestoredFilesPrepared();

  bool CanConvert() const;
  void FinishConversion(ConversionHandler const & handler, bool result);

  bool HasDuplicatedIds(kml::FileData const & fileData) const;
  bool CheckVisibility(CategoryFilterType const filter, bool isVisible) const;
  ThreadChecker m_threadChecker;

  User & m_user;
  Callbacks m_callbacks;
  MarksChangesTracker m_changesTracker;
  df::DrapeEngineSafePtr m_drapeEngine;
  AsyncLoadingCallbacks m_asyncLoadingCallbacks;
  std::atomic<bool> m_needTeardown;
  size_t m_openedEditSessionsCount = 0;
  bool m_loadBookmarksFinished = false;
  bool m_firstDrapeNotification = false;
  bool m_restoreApplying = false;
  bool m_migrationInProgress = false;
  bool m_conversionInProgress = false;
  bool m_notificationsEnabled = true;

  ScreenBase m_viewport;

  CategoriesCollection m_categories;
  kml::GroupIdCollection m_bmGroupsIdList;

  std::string m_lastCategoryUrl;
  kml::MarkGroupId m_lastEditedGroupId = kml::kInvalidMarkGroupId;
  kml::PredefinedColor m_lastColor = kml::PredefinedColor::Red;
  UserMarkLayers m_userMarkLayers;

  MarksCollection m_userMarks;
  BookmarksCollection m_bookmarks;
  TracksCollection m_tracks;

  StaticMarkPoint * m_selectionMark = nullptr;
  MyPositionMarkPoint * m_myPositionMark = nullptr;

  bool m_asyncLoadingInProgress = false;
  struct BookmarkLoaderInfo
  {
    std::string m_filename;
    bool m_isTemporaryFile = false;
    BookmarkLoaderInfo() = default;
    BookmarkLoaderInfo(std::string const & filename, bool isTemporaryFile)
      : m_filename(filename), m_isTemporaryFile(isTemporaryFile)
    {}
  };
  std::list<BookmarkLoaderInfo> m_bookmarkLoadingQueue;

  Cloud m_bookmarkCloud;
  Cloud::SynchronizationStartedHandler m_onSynchronizationStarted;
  Cloud::SynchronizationFinishedHandler m_onSynchronizationFinished;
  Cloud::RestoreRequestedHandler m_onRestoreRequested;
  Cloud::RestoredFilesPreparedHandler m_onRestoredFilesPrepared;

  BookmarkCatalog m_bookmarkCatalog;
  OnCatalogDownloadStartedHandler m_onCatalogDownloadStarted;
  OnCatalogDownloadFinishedHandler m_onCatalogDownloadFinished;
  OnCatalogImportStartedHandler m_onCatalogImportStarted;
  OnCatalogImportFinishedHandler m_onCatalogImportFinished;
  OnCatalogUploadStartedHandler m_onCatalogUploadStartedHandler;
  OnCatalogUploadFinishedHandler m_onCatalogUploadFinishedHandler;

  struct RestoringCache
  {
    std::string m_serverId;
    kml::AccessRules m_accessRules;
  };
  std::map<std::string, RestoringCache> m_restoringCache;

  bool m_testModeEnabled = false;

  DISALLOW_COPY_AND_MOVE(BookmarkManager);
};

namespace lightweight
{
namespace impl
{
bool IsBookmarksCloudEnabled();
}  // namespace impl
}  //namespace lightweight
