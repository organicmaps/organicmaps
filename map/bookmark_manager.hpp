#pragma once

#include "map/bookmark.hpp"
#include "map/route_track.hpp"
#include "map/user_mark_container.hpp"

#include "std/function.hpp"
#include "std/unique_ptr.hpp"


class Framework;
class PaintEvent;

class BookmarkManager : private noncopyable
{
  vector<BookmarkCategory *> m_categories;
  string m_lastCategoryUrl;
  string m_lastType;

  Framework & m_framework;

  vector<UserMarkContainer *> m_userMarkLayers;

  mutable double m_lastScale;

  typedef vector<BookmarkCategory *>::iterator CategoryIter;

  void SaveState() const;
  void LoadState();

public:
  BookmarkManager(Framework & f);
  ~BookmarkManager();

  void ClearItems();

  void PrepareToShutdown();

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

  /// @name Delete bookmarks category with all bookmarks.
  /// @return true if category was deleted
  void DeleteBmCategory(CategoryIter i);
  bool DeleteBmCategory(size_t index);

  typedef function<m2::AnyRectD const & (UserMarkType)> TTouchRectHolder;

  UserMark const * FindNearestUserMark(m2::AnyRectD const & rect) const;
  UserMark const * FindNearestUserMark(TTouchRectHolder const & holder) const;

  /// Additional layer methods
  bool UserMarksIsVisible(UserMarkType type) const;
  UserMarksController & UserMarksRequestController(UserMarkType type);
  void UserMarksReleaseController(UserMarksController & controller);

private:
  UserMarkContainer const * FindUserMarksContainer(UserMarkType type) const;
  UserMarkContainer * FindUserMarksContainer(UserMarkType type);
};

class UserMarkControllerGuard
{
public:
  UserMarkControllerGuard(BookmarkManager & mng, UserMarkType type);
  ~UserMarkControllerGuard();

  BookmarkManager & m_mng;
  UserMarksController & m_controller;
};
