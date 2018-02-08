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

class BookmarkManager final : public df::UserMarksProvider
{
  using CategoriesCollection = std::map<df::MarkGroupID, std::unique_ptr<BookmarkCategory>>;
  using MarksCollection = std::map<df::MarkID, std::unique_ptr<UserMark>>;
  using BookmarksCollection = std::map<df::MarkID, std::unique_ptr<Bookmark>>;
  using TracksCollection = std::map<df::LineID, std::unique_ptr<Track>>;

  using CategoryIter = CategoriesCollection::iterator;
  using GroupIdList = std::vector<df::MarkGroupID>;

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

  template <typename UserMarkT>
  UserMarkT * CreateUserMark(m2::PointD const & ptOrg)
  {
    auto mark = std::make_unique<UserMarkT>(ptOrg);
    auto * m = mark.get();
    auto const markId = m->GetId();
    auto const groupId = static_cast<df::MarkGroupID>(m->GetMarkType());
    ASSERT(m_userMarks.count(markId) == 0, ());
    ASSERT_LESS(groupId, m_userMarkLayers.size(), ());
    m_userMarks.emplace(markId, std::move(mark));
    m_createdMarks.insert(markId);
    m_userMarkLayers[groupId]->AttachUserMark(markId);
    return m;
  }

  template <typename UserMarkT>
  UserMarkT * GetMarkForEdit(df::MarkID markId)
  {
    auto * mark = GetUserMarkForEdit(markId);
    ASSERT(dynamic_cast<UserMarkT *>(mark) != nullptr, ());
    return static_cast<UserMarkT *>(mark);
  }

  template <typename UserMarkT>
  UserMarkT const * GetMark(df::MarkID markId) const
  {
    auto * mark = GetUserMark(markId);
    ASSERT(dynamic_cast<UserMarkT const *>(mark) != nullptr, ());
    return static_cast<UserMarkT const *>(mark);
  }

  template <typename UserMarkT, typename F>
  void DeleteUserMarks(UserMark::Type type, F deletePredicate)
  {
    std::list<df::MarkID> marksToDelete;
    for (auto markId : GetUserMarkIds(type))
    {
      if (deletePredicate(GetMark<UserMarkT>(markId)))
        marksToDelete.push_back(markId);
    }
    // Delete after iterating to avoid iterators invalidation issues.
    for (auto markId : marksToDelete)
      DeleteUserMark(markId);
  };

  UserMark const * GetUserMark(df::MarkID markID) const;
  UserMark * GetUserMarkForEdit(df::MarkID markID);
  void DeleteUserMark(df::MarkID markId);

  Bookmark * CreateBookmark(m2::PointD const & ptOrg, BookmarkData & bm);
  Bookmark * CreateBookmark(m2::PointD const & ptOrg, BookmarkData & bm, df::MarkGroupID groupID);
  Bookmark const * GetBookmark(df::MarkID markID) const;
  Bookmark * GetBookmarkForEdit(df::MarkID markID);
  void AttachBookmark(df::MarkID bmId, df::MarkGroupID groupID);
  void DetachBookmark(df::MarkID bmId, df::MarkGroupID groupID);
  void DeleteBookmark(df::MarkID bmId);

  Track * CreateTrack(m2::PolylineD const & polyline, Track::Params const & p);
  Track const * GetTrack(df::LineID trackID) const;
  void AttachTrack(df::LineID trackID, df::MarkGroupID groupID);
  void DetachTrack(df::LineID trackID, df::MarkGroupID groupID);
  void DeleteTrack(df::LineID trackID);

  //////////////////
  void ClearGroup(df::MarkGroupID groupID);

  void NotifyChanges(df::MarkGroupID groupID);

  df::MarkIDSet const & GetUserMarkIds(df::MarkGroupID groupID) const;
  df::LineIDSet const & GetTrackIds(df::MarkGroupID groupID) const;

  std::string const & GetCategoryName(df::MarkGroupID categoryId) const;
  void SetCategoryName(df::MarkGroupID categoryId, std::string const & name);

  UserMark const * FindMarkInRect(df::MarkGroupID groupId, m2::AnyRectD const & rect, double & d) const;

  void SetIsVisible(df::MarkGroupID groupId, bool visible);
  bool IsVisible(df::MarkGroupID groupId) const;

  /// Uses the same file name from which was loaded, or
  /// creates unique file name on first save and uses it every time.
  bool SaveToKMLFile(df::MarkGroupID groupID);
  /// @name This fuctions is public for unit tests only.
  /// You don't need to call it from client code.
  void SaveToKML(BookmarkCategory * group, std::ostream & s);

  std::string const & GetCategoryFileName(df::MarkGroupID categoryId) const;
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

  /// Client should know where it moves bookmark
  void MoveBookmark(df::MarkID bmID, df::MarkGroupID curGroupID, df::MarkGroupID newGroupID);
  void UpdateBookmark(df::MarkID bmId, BookmarkData const & bm);

  df::MarkGroupID LastEditedBMCategory();
  std::string LastEditedBMType() const;

  GroupIdList const & GetBmGroupsIdList() const { return m_bmGroupsIdList; }
  bool HasBmCategory(df::MarkGroupID groupID) const;

  df::MarkGroupID CreateBmCategory(std::string const & name);

  /// @name Delete bookmarks category with all bookmarks.
  /// @return true if category was deleted
  bool DeleteBmCategory(df::MarkGroupID groupID);

  using TTouchRectHolder = function<m2::AnyRectD(UserMark::Type)>;

  UserMark const * FindNearestUserMark(m2::AnyRectD const & rect) const;
  UserMark const * FindNearestUserMark(TTouchRectHolder const & holder) const;

  StaticMarkPoint & SelectionMark() { return *m_selectionMark; }
  StaticMarkPoint const & SelectionMark() const { return *m_selectionMark; }
  MyPositionMarkPoint & MyPositionMark() { return *m_myPositionMark; }
  MyPositionMarkPoint const & MyPositionMark() const { return *m_myPositionMark; }

  bool IsAsyncLoadingInProgress() const { return m_asyncLoadingInProgress; }

  // UserMarksProvider
  df::GroupIDSet const & GetDirtyGroupIds() const override;
  bool IsGroupVisible(df::MarkGroupID groupID) const override;
  bool IsGroupVisiblityChanged(df::MarkGroupID groupID) const override;
  df::MarkIDSet const & GetGroupPointIds(df::MarkGroupID groupId) const override;
  df::LineIDSet const & GetGroupLineIds(df::MarkGroupID groupId) const override;
  df::MarkIDSet const & GetCreatedMarkIds() const override;
  df::MarkIDSet const & GetRemovedMarkIds() const override;
  df::MarkIDSet const & GetUpdatedMarkIds() const override;
  df::UserPointMark const * GetUserPointMark(df::MarkID markID) const override;
  df::UserLineMark const * GetUserLineMark(df::LineID lineID) const override;

private:
  using KMLDataCollection = std::vector<std::unique_ptr<KMLData>>;

  static bool IsBookmark(df::MarkGroupID groupId) { return groupId >= UserMark::BOOKMARK; }
  static bool IsBookmark(df::MarkID markID) { return UserMark::GetMarkType(markID) == UserMark::BOOKMARK; }
  UserMark const * GetMark(df::MarkID markID) const;
  void FindDirtyGroups();

  UserMarkContainer const * FindContainer(df::MarkGroupID containerId) const;
  UserMarkContainer * FindContainer(df::MarkGroupID containerId);
  BookmarkCategory * GetBmCategory(df::MarkGroupID categoryId) const;

  Bookmark * AddBookmark(std::unique_ptr<Bookmark> && bookmark);
  Track * AddTrack(std::unique_ptr<Track> && track);

  void SaveState() const;
  void LoadState();
  void CreateCategories(KMLDataCollection && dataCollection);
  void NotifyAboutStartAsyncLoading();
  void NotifyAboutFinishAsyncLoading(std::shared_ptr<KMLDataCollection> && collection);
  boost::optional<std::string> GetKMLPath(std::string const & filePath);
  void NotifyAboutFile(bool success, std::string const & filePath, bool isTemporaryFile);
  void LoadBookmarkRoutine(std::string const & filePath, bool isTemporaryFile);

  void SendBookmarksChanges();
  void GetBookmarksData(df::MarkIDSet const & markIds,
                        std::vector<std::pair<df::MarkID, BookmarkData>> & data) const;

  Callbacks m_callbacks;

  df::DrapeEngineSafePtr m_drapeEngine;
  AsyncLoadingCallbacks m_asyncLoadingCallbacks;
  std::atomic<bool> m_needTeardown;
  df::MarkGroupID m_nextGroupID;
  bool m_loadBookmarksFinished = false;

  ScreenBase m_viewport;

  CategoriesCollection m_categories;
  df::GroupIDList m_bmGroupsIdList;
  df::GroupIDSet m_dirtyGroups;
  std::string m_lastCategoryUrl;
  std::string m_lastType;
  UserMarkLayers m_userMarkLayers;

  MarksCollection m_userMarks;
  BookmarksCollection m_bookmarks;
  TracksCollection m_tracks;
  df::MarkIDSet m_createdMarks;
  df::MarkIDSet m_removedMarks;
  df::MarkIDSet m_updatedMarks;

  StaticMarkPoint * m_selectionMark = nullptr;
  MyPositionMarkPoint * m_myPositionMark = nullptr;

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
