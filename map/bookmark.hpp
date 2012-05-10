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

public:
  Bookmark() {}
  Bookmark(m2::PointD const & org, string const & name)
    : m_org(org), m_name(name)
  {
  }

  m2::PointD GetOrg() const { return m_org; }
  string const & GetName() const { return m_name; }
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

  bool IsVisible() const { return m_visible; }
  string GetName() const { return m_name; }

  inline size_t GetBookmarksCount() const { return m_bookmarks.size(); }
  Bookmark const * GetBookmark(size_t index) const;
  void RemoveBookmark(size_t index);

  void LoadFromKML(ReaderPtr<Reader> const & reader);
  void SaveToKML(ostream & s);
};
