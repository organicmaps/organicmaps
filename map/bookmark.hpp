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
  string m_type;    ///< Now it stores bookmark color (category style).
  double m_scale;   ///< Viewport scale. -1.0 - is a default value (no scale set).

public:
  Bookmark() {}
  Bookmark(m2::PointD const & org, string const & name, string const & type)
    : m_org(org), m_name(name), m_type(type), m_scale(-1.0)
  {
  }

  m2::PointD GetOrg() const { return m_org; }
  string const & GetName() const { return m_name; }
  /// @return Now its a bookmark color.
  string const & GetType() const { return m_type; }
  m2::RectD GetViewport() const { return m2::RectD(m_org, m_org); }

  double GetScale() const { return m_scale; }
  void SetScale(double scale) { m_scale = scale; }
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

  /// @name Theese functions are called from Framework only.
  //@{
  void AddBookmark(Bookmark const & bm, double scale);
  void ReplaceBookmark(size_t index, Bookmark const & bm, double scale);
  //@}

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

  /// @name Theese fuctions are public for unit tests only.
  /// You don't need to call them from client code.
  //@{
  void LoadFromKML(ReaderPtr<Reader> const & reader);
  void SaveToKML(ostream & s);

  /// Uses the same file name from which was loaded, or
  /// creates unique file name on first save and uses it every time.
  bool SaveToKMLFile();

  /// @return 0 in the case of error
  static BookmarkCategory * CreateFromKMLFile(string const & file);

  static string GenerateUniqueFileName(const string & path, string const & name);
  //@}
};

/// <category index, bookmark index>
typedef pair<int, int> BookmarkAndCategory;
inline BookmarkAndCategory MakeEmptyBookmarkAndCategory()
{
  return BookmarkAndCategory(int(-1), int(-1));
}

inline bool IsValid(BookmarkAndCategory const & bmc)
{
  return (bmc.first >= 0 && bmc.second >= 0);
}
