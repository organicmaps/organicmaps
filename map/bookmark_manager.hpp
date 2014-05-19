#pragma once
#include "bookmark.hpp"
#include "user_mark_container.hpp"
#include "user_mark_dl_cache.hpp"

class Framework;
class PaintEvent;
namespace graphics { class Screen; }


class BookmarkManager : private noncopyable
{
  vector<BookmarkCategory *> m_categories;
  string m_lastCategoryUrl;
  string m_lastType;

  Framework & m_framework;

  vector<UserMarkContainer * > m_userMarkLayers;
  UserMark const * m_activeMark;

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
  void ReplaceBookmark(size_t catIndex, size_t bmIndex, BookmarkData const & bm);

  size_t LastEditedBMCategory();
  string LastEditedBMType() const;

  inline size_t GetBmCategoriesCount() const { return m_categories.size(); }

  /// @returns 0 if category is not found
  BookmarkCategory * GetBmCategory(size_t index) const;

  size_t CreateBmCategory(string const & name);
  void DrawItems(shared_ptr<PaintEvent> const & e) const;

  /// @name Delete bookmarks category with all bookmarks.
  /// @return true if category was deleted
  void DeleteBmCategory(CategoryIter i);
  bool DeleteBmCategory(size_t index);

  void ActivateMark(UserMark const * mark);
  UserMark const * FindNearestUserMark(m2::AnyRectD const & rect);

  /// Additional layer methods
  void UserMarksSetVisible(UserMarkContainer::Type type, bool isVisible);
  bool UserMarksIsVisible(UserMarkContainer::Type type) const;
  UserMark * UserMarksAddMark(UserMarkContainer::Type type, m2::PointD const & ptOrg);
  void UserMarksClear(UserMarkContainer::Type type);
  UserMarkContainer::Controller & UserMarksGetController(UserMarkContainer::Type type);

  void SetScreen(graphics::Screen * screen);
  void ResetScreen();

private:
  UserMarkContainer const * FindUserMarksContainer(UserMarkContainer::Type type) const;
  UserMarkContainer * FindUserMarksContainer(UserMarkContainer::Type type);

  UserMarkDLCache * m_cache;
};
