#pragma once

#include "../coding/reader.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../base/timer.hpp"

#include "../std/string.hpp"
#include "../std/noncopyable.hpp"
#include "../std/iostream.hpp"

#include "track.hpp"


class Bookmark
{
  m2::PointD m_org;
  string m_name;
  string m_description;
  string m_type;    ///< Now it stores bookmark color (category style).
  double m_scale;   ///< Viewport scale. -1.0 - is a default value (no scale set).
  time_t m_timeStamp;

public:
  Bookmark() : m_scale(-1.0), m_timeStamp(my::INVALID_TIME_STAMP) {}
  Bookmark(m2::PointD const & org, string const & name, string const & type)
    : m_org(org), m_name(name), m_type(type), m_scale(-1.0), m_timeStamp(my::INVALID_TIME_STAMP)
  {
  }

  m2::PointD const & GetOrg() const { return m_org; }
  string const & GetName() const { return m_name; }
  //void SetName(string const & name) { m_name = name; }
  /// @return Now its a bookmark color - name of icon file
  string const & GetType() const { return m_type; }
  //void SetType(string const & type) { m_type = type; }
  m2::RectD GetViewport() const { return m2::RectD(m_org, m_org); }

  string const & GetDescription() const { return m_description; }
  void SetDescription(string const & description) { m_description = description; }

  /// @return my::INVALID_TIME_STAMP if bookmark has no timestamp
  time_t GetTimeStamp() const { return m_timeStamp; }
  void SetTimeStamp(time_t timeStamp) { m_timeStamp = timeStamp; }

  double GetScale() const { return m_scale; }
  void SetScale(double scale) { m_scale = scale; }
};

class BookmarkCategory : private noncopyable
{
  /// @name Data
  //@{
  vector<Bookmark *> m_bookmarks;
  vector<Track *> m_tracks;
  //@}

  string m_name;
  bool m_visible;
  /// Stores file name from which category was loaded
  string m_file;

  /// This function is called when bookmark is editing or replacing.
  /// We need to assign private params to the newly created bookmark from the old one.
  void AssignPrivateParams(size_t index, Bookmark & bm) const;

public:
  BookmarkCategory(string const & name) : m_name(name), m_visible(true) {}
  ~BookmarkCategory();

  void ClearBookmarks();
  void ClearTracks();

  static string GetDefaultType();

  /// @name Theese functions are called from Framework only.
  //@{
  void AddBookmark(Bookmark const & bm);
  void ReplaceBookmark(size_t index, Bookmark const & bm);
  //@}

  /// @name Track routines
  //@{
  void AddTrack(Track const & track);
  Track * GetTrack(size_t index) const;
  inline size_t GetTracksCount()    const { return m_tracks.size(); }
  //@}

  void SetVisible(bool isVisible) { m_visible = isVisible; }
  bool IsVisible() const { return m_visible; }

  void SetName(string const & name) { m_name = name; }
  string const & GetName() const { return m_name; }
  string const & GetFileName() const { return m_file; }

  inline size_t GetBookmarksCount() const { return m_bookmarks.size(); }

  Bookmark const * GetBookmark(size_t index) const;
  Bookmark * GetBookmark(size_t index);
  /// @param[in] distance in metres between orgs
  /// @returns -1 or index of found bookmark
  int GetBookmark(m2::PointD const org, double const squareDistance) const;

  void DeleteBookmark(size_t index);

  /// @name Theese fuctions are public for unit tests only.
  /// You don't need to call them from client code.
  //@{
  bool LoadFromKML(ReaderPtr<Reader> const & reader);
  void SaveToKML(ostream & s);

  /// Uses the same file name from which was loaded, or
  /// creates unique file name on first save and uses it every time.
  bool SaveToKMLFile();

  /// @return 0 in the case of error
  static BookmarkCategory * CreateFromKMLFile(string const & file);

  /// Get valid file name from input (remove illegal symbols).
  static string RemoveInvalidSymbols(string const & name);
  /// Get unique bookmark file name from path and valid file name.
  static string GenerateUniqueFileName(const string & path, string name);
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
