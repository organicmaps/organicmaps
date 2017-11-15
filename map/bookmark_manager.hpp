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

class BookmarkManager final
{
  using CategoriesCollection = std::vector<std::unique_ptr<BookmarkCategory>>;
  using CategoryIter = CategoriesCollection::iterator;

  using UserMarkLayers = std::vector<std::unique_ptr<UserMarkContainer>>;
  using GetStringsBundleFn = std::function<StringsBundle const &()>;

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

  explicit BookmarkManager(GetStringsBundleFn && getStringsBundleFn);
  ~BookmarkManager();

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

  inline size_t GetBmCategoriesCount() const { return m_categories.size(); }

  /// @returns 0 if category is not found
  BookmarkCategory * GetBmCategory(size_t index) const;

  size_t CreateBmCategory(std::string const & name);

  /// @name Delete bookmarks category with all bookmarks.
  /// @return true if category was deleted
  void DeleteBmCategory(CategoryIter i);
  bool DeleteBmCategory(size_t index);

  using TTouchRectHolder = function<m2::AnyRectD(UserMark::Type)>;

  UserMark const * FindNearestUserMark(m2::AnyRectD const & rect) const;
  UserMark const * FindNearestUserMark(TTouchRectHolder const & holder) const;

  /// Additional layer methods
  bool UserMarksIsVisible(UserMark::Type type) const;
  UserMarksController & GetUserMarksController(UserMark::Type type);

  std::unique_ptr<StaticMarkPoint> & SelectionMark();
  std::unique_ptr<StaticMarkPoint> const & SelectionMark() const;
  std::unique_ptr<MyPositionMarkPoint> & MyPositionMark();
  std::unique_ptr<MyPositionMarkPoint> const & MyPositionMark() const;

  bool IsAsyncLoadingInProgress() const { return m_asyncLoadingInProgress; }

private:
  UserMarkContainer const * FindUserMarksContainer(UserMark::Type type) const;
  UserMarkContainer * FindUserMarksContainer(UserMark::Type type);

  void SaveState() const;
  void LoadState();
  void MergeCategories(CategoriesCollection && newCategories);
  void NotifyAboutStartAsyncLoading();
  void NotifyAboutFinishAsyncLoading(std::shared_ptr<CategoriesCollection> && collection);
  boost::optional<std::string> GetKMLPath(std::string const & filePath);
  void NotifyAboutFile(bool success, std::string const & filePath, bool isTemporaryFile);
  void LoadBookmarkRoutine(std::string const & filePath, bool isTemporaryFile);

  GetStringsBundleFn m_getStringsBundle;
  df::DrapeEngineSafePtr m_drapeEngine;
  AsyncLoadingCallbacks m_asyncLoadingCallbacks;
  std::atomic<bool> m_needTeardown;
  bool m_loadBookmarksFinished = false;

  ScreenBase m_viewport;

  CategoriesCollection m_categories;
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
  UserMarkNotificationGuard(BookmarkManager & mng, UserMark::Type type);
  ~UserMarkNotificationGuard();

  UserMarksController & m_controller;
};
