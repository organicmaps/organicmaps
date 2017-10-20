#pragma once

#include "map/bookmark.hpp"
#include "map/user_mark_container.hpp"

#include "geometry/any_rect2d.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class Framework;
class PaintEvent;

class BookmarkManager : private noncopyable
{
  using CategoriesCollection = std::vector<std::unique_ptr<BookmarkCategory>>;
  using CategoryIter = CategoriesCollection::iterator;

  using UserMarkLayers = std::vector<std::unique_ptr<UserMarkContainer>>;

  CategoriesCollection m_categories;

  std::string m_lastCategoryUrl;
  std::string m_lastType;

  Framework & m_framework;

  UserMarkLayers m_userMarkLayers;

  void SaveState() const;
  void LoadState();

public:
  BookmarkManager(Framework & f);
  ~BookmarkManager();

  void ClearCategories();

  /// Scans and loads all kml files with bookmarks in WritableDir.
  void LoadBookmarks();
  void LoadBookmark(string const & filePath);

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

private:
  UserMarkContainer const * FindUserMarksContainer(UserMark::Type type) const;
  UserMarkContainer * FindUserMarksContainer(UserMark::Type type);
};

class UserMarkNotificationGuard
{
public:
  UserMarkNotificationGuard(BookmarkManager & mng, UserMark::Type type);
  ~UserMarkNotificationGuard();

  UserMarksController & m_controller;
};
