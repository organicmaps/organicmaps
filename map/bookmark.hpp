#pragma once

#include "../coding/reader.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../std/string.hpp"
#include "../std/noncopyable.hpp"
#include "../std/iostream.hpp"


class Bookmark
{
  m2::PointD m_org;
  string m_name;
  /// Now it stores bookmark color
  string m_type;

public:
  Bookmark() {}
  Bookmark(m2::PointD const & org, string const & name, string const & type)
    : m_org(org), m_name(name), m_type(type)
  {
  }

  m2::PointD GetOrg() const { return m_org; }
  string const & GetName() const { return m_name; }
  /// Now it returns bookmark color
  string const & GetType() const { return m_type; }
  m2::RectD GetViewport() const { return m2::RectD(m_org, m_org); }
};

class BookmarkCategory : private noncopyable
{
  string m_name;
  vector<Bookmark *> m_bookmarks;
  bool m_visible;

public:
  BookmarkCategory(string const & name) : m_name(name), m_visible(true) {}
  ~BookmarkCategory();

  void ClearBookmarks();

  void AddBookmark(Bookmark const & bm);

  void SetVisible(bool isVisible) { m_visible = isVisible; }
  bool IsVisible() const { return m_visible; }
  string GetName() const { return m_name; }

  inline size_t GetBookmarksCount() const { return m_bookmarks.size(); }
  Bookmark const * GetBookmark(size_t index) const;
  void DeleteBookmark(size_t index);

  void LoadFromKML(ReaderPtr<Reader> const & reader);
  void SaveToKML(ostream & s);
};

/// Non-const category is needed to "edit" bookmark (actually, re-add it)
typedef pair<BookmarkCategory *, Bookmark const *> BookmarkAndCategory;
