#pragma once
#include "map/bookmark.hpp"
#include "map/route_track.hpp"
#include "map/user_mark_container.hpp"
#include "map/user_mark_dl_cache.hpp"

#include "std/function.hpp"
#include "std/unique_ptr.hpp"


class Framework;
class PaintEvent;
namespace graphics { class Screen; }
namespace rg { class RouteRenderer; }

class BookmarkManager : private noncopyable
{
  vector<BookmarkCategory *> m_categories;
  string m_lastCategoryUrl;
  string m_lastType;

  Framework & m_framework;

  vector<UserMarkContainer * > m_userMarkLayers;

  graphics::Screen * m_bmScreen;
  mutable double m_lastScale;

  typedef vector<BookmarkCategory *>::iterator CategoryIter;

  void DrawCategory(BookmarkCategory const * cat, PaintOverlayEvent const & e) const;

  void SaveState() const;
  void LoadState();

public:
  BookmarkManager(Framework & f);
  ~BookmarkManager();

  void ClearItems();

  /// Scans and loads all kml files with bookmarks in WritableDir.
  void LoadBookmarks();
  void LoadBookmark(string const & filePath);

  /// Client should know where it adds bookmark
  size_t AddBookmark(size_t categoryIndex, m2::PointD const & ptOrg, BookmarkData & bm);
  /// Client should know where it moves bookmark
  size_t MoveBookmark(size_t bmIndex, size_t curCatIndex, size_t newCatIndex);
  void ReplaceBookmark(size_t catIndex, size_t bmIndex, BookmarkData const & bm);

  size_t LastEditedBMCategory();
  string LastEditedBMType() const;

  inline size_t GetBmCategoriesCount() const { return m_categories.size(); }

  /// @returns 0 if category is not found
  BookmarkCategory * GetBmCategory(size_t index) const;

  size_t CreateBmCategory(string const & name);
  void DrawItems(Drawer * drawer) const;

  /// @name Delete bookmarks category with all bookmarks.
  /// @return true if category was deleted
  void DeleteBmCategory(CategoryIter i);
  bool DeleteBmCategory(size_t index);

  void ActivateMark(UserMark const * mark, bool needAnim);
  bool UserMarkHasActive() const;
  bool IsUserMarkActive(UserMark const * container) const;

  typedef function<m2::AnyRectD const & (UserMarkContainer::Type)> TTouchRectHolder;

  UserMark const * FindNearestUserMark(m2::AnyRectD const & rect) const;
  UserMark const * FindNearestUserMark(TTouchRectHolder const & holder) const;

  /// Additional layer methods
  void UserMarksSetVisible(UserMarkContainer::Type type, bool isVisible);
  bool UserMarksIsVisible(UserMarkContainer::Type type) const;
  void UserMarksSetDrawable(UserMarkContainer::Type type, bool isDrawable);
  void UserMarksIsDrawable(UserMarkContainer::Type type);
  UserMark * UserMarksAddMark(UserMarkContainer::Type type, m2::PointD const & ptOrg);
  void UserMarksClear(UserMarkContainer::Type type, size_t skipCount = 0);
  UserMarkContainer::Controller & UserMarksGetController(UserMarkContainer::Type type);

  void SetScreen(graphics::Screen * screen);
  void ResetScreen();

  void SetRouteTrack(m2::PolylineD const & routePolyline, vector<double> const & turns,
                     graphics::Color const & color);
  void ResetRouteTrack();
  void UpdateRouteDistanceFromBegin(double distance);

private:
  UserMarkContainer const * FindUserMarksContainer(UserMarkContainer::Type type) const;
  UserMarkContainer * FindUserMarksContainer(UserMarkContainer::Type type);

  UserMarkDLCache * m_cache;

  SelectionContainer m_selection;

  unique_ptr<rg::RouteRenderer> m_routeRenderer;
};
