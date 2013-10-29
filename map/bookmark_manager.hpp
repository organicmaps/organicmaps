#pragma once
#include "bookmark.hpp"


/// Special number for additional layer category.
const int additionalLayerCategory = -2;

class Framework;
class PaintEvent;
namespace graphics { class Screen; }


class BookmarkManager : private noncopyable
{
  vector<BookmarkCategory *> m_categories;
  string m_lastCategoryUrl;
  string m_lastType;

  Framework & m_framework;

  BookmarkCategory * m_additionalPoiLayer;

  graphics::Screen * m_bmScreen;
  mutable double m_lastScale;

  typedef vector<BookmarkCategory *>::iterator CategoryIter;

  void DrawCategory(BookmarkCategory const * cat, shared_ptr<PaintEvent> const & e) const;

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
  size_t AddBookmark(size_t categoryIndex, Bookmark & bm);
  void ReplaceBookmark(size_t catIndex, size_t bmIndex, Bookmark const & bm);

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

  /// Additional layer methods
  void AdditionalPoiLayerSetInvisible();
  void AdditionalPoiLayerSetVisible();
  void AdditionalPoiLayerAddPoi(Bookmark const & bm);
  Bookmark const * AdditionalPoiLayerGetBookmark(size_t index) const;
  Bookmark * AdditionalPoiLayerGetBookmark(size_t index);
  void AdditionalPoiLayerClear();
  bool IsAdditionalLayerPoi(const BookmarkAndCategory & bm) const;
  bool AdditionalLayerIsVisible() const { return m_additionalPoiLayer->IsVisible(); }
  size_t AdditionalLayerNumberOfPoi() const { return m_additionalPoiLayer->GetBookmarksCount(); }

  void SetScreen(graphics::Screen * screen);
  void DeleteScreen();
};
