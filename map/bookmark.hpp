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
  /// Stores file name from which category was loaded
  string m_file;

public:
  BookmarkCategory(string const & name) : m_name(name), m_visible(true) {}
  ~BookmarkCategory();

  void ClearBookmarks();

  void AddBookmark(Bookmark const & bm);

  void SetVisible(bool isVisible) { m_visible = isVisible; }
  bool IsVisible() const { return m_visible; }
  void SetName(string const & name) { m_name = name; }
  string GetName() const { return m_name; }
  string GetFileName() const { return m_file; }

  inline size_t GetBookmarksCount() const { return m_bookmarks.size(); }
  Bookmark const * GetBookmark(size_t index) const;
  /// @param[in] distance in metres between orgs
  /// @returns -1 or index of found bookmark
  int GetBookmark(m2::PointD const org, double const squareDistance) const;
  void DeleteBookmark(size_t index);
  void ReplaceBookmark(size_t index, Bookmark const & bm);

  void LoadFromKML(ReaderPtr<Reader> const & reader);
  void SaveToKML(ostream & s);
  /// @return 0 in the case of error
  static BookmarkCategory * CreateFromKMLFile(string const & file);
  /// Uses the same file name from which was loaded, or
  /// creates unique file name on first save and uses it every time
  /// @param[in] path directory name where to save
  bool SaveToKMLFileAtPath(string const & path);

  static string GenerateUniqueFileName(const string & path, string const & name);
};

/// <category name, bookmark index>
typedef pair<string, int> BookmarkAndCategory;
inline BookmarkAndCategory MakeEmptyBookmarkAndCategory()
{
  return BookmarkAndCategory(string(), int(-1));
}

inline bool IsValid(BookmarkAndCategory const & bmc)
{
  return (!bmc.first.empty() && bmc.second >= 0);
}
