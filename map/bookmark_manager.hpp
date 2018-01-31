#pragma once

#include "map/bookmark.hpp"
#include "map/user_mark_container.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/macros.hpp"
#include "base/strings_bundle.hpp"

#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include <boost/optional.hpp>

class BookmarkManager final : public UserMarkManager
{
  using CategoriesCollection = std::map<size_t, std::unique_ptr<BookmarkCategory>>;
  using CategoryIter = CategoriesCollection::iterator;
  using CategoriesIdList = std::vector<size_t>;

  using UserMarkLayers = std::vector<std::unique_ptr<UserMarkContainer>>;
public:

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
    using CreatedBookmarksCallback = std::function<void(std::vector<std::pair<df::MarkID, BookmarkData>> const &)>;
    using UpdatedBookmarksCallback = std::function<void(std::vector<std::pair<df::MarkID, BookmarkData>> const &)>;
    using DeletedBookmarksCallback = std::function<void(std::vector<df::MarkID> const &)>;

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

  explicit BookmarkManager(Callbacks && callbacks);
  ~BookmarkManager();

  //////////////////
  float GetPointDepth(size_t categoryId) const override;
  void NotifyChanges(size_t categoryId);
  size_t GetUserMarkCount(size_t categoryId) const;
  UserMark const * GetUserMark(size_t categoryId, size_t index) const;
  UserMark * GetUserMarkForEdit(size_t categoryId, size_t index);
  void DeleteUserMark(size_t categoryId, size_t index);
  void ClearUserMarks(size_t categoryId);
  Bookmark const * GetBookmark(size_t categoryId, size_t bmIndex) const;
  Bookmark * GetBookmarkForEdit(size_t categoryId, size_t bmIndex);
  size_t GetTracksCount(size_t categoryId) const;
  Track const * GetTrack(size_t categoryId, size_t index) const;
  void DeleteTrack(size_t categoryId, size_t index);
  bool SaveToKMLFile(size_t categoryId);
  std::string const & GetCategoryName(size_t categoryId) const;
  void SetCategoryName(size_t categoryId, std::string const & name);
  std::string const & GetCategoryFileName(size_t categoryId) const;

  UserMark const * FindMarkInRect(size_t categoryId, m2::AnyRectD const & rect, double & d) const;

  UserMark * CreateUserMark(size_t categoryId, m2::PointD const & ptOrg);

  void SetIsVisible(size_t categoryId, bool visible);
  bool IsVisible(size_t categoryId) const;

  /// Get valid file name from input (remove illegal symbols).
  static std::string RemoveInvalidSymbols(std::string const & name);
  /// Get unique bookmark file name from path and valid file name.
  static std::string GenerateUniqueFileName(const std::string & path, std::string name);
  //////////////////

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void UpdateViewport(ScreenBase const & screen);
  void SetAsyncLoadingCallbacks(AsyncLoadingCallbacks && callbacks);
  void Teardown();

  void ClearCategories();

  /// Scans and loads all kml files with bookmarks in WritableDir.
  void LoadBookmarks();
  void LoadBookmark(std::string const & filePath, bool isTemporaryFile);

  void InitBookmarks();

  /// Client should know where it adds bookmark
  size_t AddBookmark(size_t categoryIndex, m2::PointD const & ptOrg, BookmarkData & bm);
  /// Client should know where it moves bookmark
  size_t MoveBookmark(size_t bmIndex, size_t curCatIndex, size_t newCatIndex);
  void ReplaceBookmark(size_t catIndex, size_t bmIndex, BookmarkData const & bm);

  size_t LastEditedBMCategory();
  std::string LastEditedBMType() const;

  CategoriesIdList const & GetBmCategoriesIds() const { return m_categoriesIdList; }

  /// @returns 0 if category is not found
  BookmarkCategory * GetBmCategory(size_t categoryId) const;

  size_t CreateBmCategory(std::string const & name);

  /// @name Delete bookmarks category with all bookmarks.
  /// @return true if category was deleted
  bool DeleteBmCategory(size_t categoryId);

  using TTouchRectHolder = function<m2::AnyRectD(UserMark::Type)>;

  Bookmark const * GetBookmark(df::MarkID id) const;
  Bookmark const * GetBookmark(df::MarkID id, size_t & catIndex, size_t & bmIndex) const;

  UserMark const * FindNearestUserMark(m2::AnyRectD const & rect) const;
  UserMark const * FindNearestUserMark(TTouchRectHolder const & holder) const;

  std::unique_ptr<StaticMarkPoint> & SelectionMark();
  std::unique_ptr<StaticMarkPoint> const & SelectionMark() const;
  std::unique_ptr<MyPositionMarkPoint> & MyPositionMark();
  std::unique_ptr<MyPositionMarkPoint> const & MyPositionMark() const;

  bool IsAsyncLoadingInProgress() const { return m_asyncLoadingInProgress; }

private:
  UserMarkContainer const * FindContainer(size_t containerId) const;
  UserMarkContainer * FindContainer(size_t containerId);

  void SaveState() const;
  void LoadState();
  void MergeCategories(CategoriesCollection && newCategories);
  void NotifyAboutStartAsyncLoading();
  void NotifyAboutFinishAsyncLoading(std::shared_ptr<CategoriesCollection> && collection);
  boost::optional<std::string> GetKMLPath(std::string const & filePath);
  void NotifyAboutFile(bool success, std::string const & filePath, bool isTemporaryFile);
  void LoadBookmarkRoutine(std::string const & filePath, bool isTemporaryFile);

  void OnCreateUserMarks(UserMarkContainer const & container, df::IDCollection const & markIds);
  void OnUpdateUserMarks(UserMarkContainer const & container, df::IDCollection const & markIds);
  void OnDeleteUserMarks(UserMarkContainer const & container, df::IDCollection const & markIds);
  void GetBookmarksData(UserMarkContainer const & container, df::IDCollection const & markIds,
                        std::vector<std::pair<df::MarkID, BookmarkData>> & data) const;

  Callbacks m_callbacks;
  UserMarkContainer::Listeners m_bookmarksListeners;

  df::DrapeEngineSafePtr m_drapeEngine;
  AsyncLoadingCallbacks m_asyncLoadingCallbacks;
  std::atomic<bool> m_needTeardown;
  std::atomic<size_t> m_nextCategoryId;
  bool m_loadBookmarksFinished = false;

  ScreenBase m_viewport;

  CategoriesCollection m_categories;
  CategoriesIdList m_categoriesIdList;
  std::string m_lastCategoryUrl;
  std::string m_lastType;
  UserMarkLayers m_userMarkLayers;

  std::unique_ptr<StaticMarkPoint> m_selectionMark;
  std::unique_ptr<MyPositionMarkPoint> m_myPositionMark;

  bool m_asyncLoadingInProgress = false;
  struct BookmarkLoaderInfo
  {
    std::string m_filename;
    bool m_isTemporaryFile = false;
    BookmarkLoaderInfo() {}
    BookmarkLoaderInfo(std::string const & filename, bool isTemporaryFile)
      : m_filename(filename), m_isTemporaryFile(isTemporaryFile)
    {}
  };
  std::list<BookmarkLoaderInfo> m_bookmarkLoadingQueue;

  DISALLOW_COPY_AND_MOVE(BookmarkManager);
};

class UserMarkNotificationGuard
{
public:
  UserMarkNotificationGuard(BookmarkManager & mng, size_t containerId);
  ~UserMarkNotificationGuard();

  BookmarkManager & m_manager;
  size_t m_containerId;
};
