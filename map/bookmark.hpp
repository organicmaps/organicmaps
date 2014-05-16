#pragma once

#include "user_mark.hpp"
#include "user_mark_container.hpp"

#include "../coding/reader.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../base/timer.hpp"

#include "../std/string.hpp"
#include "../std/noncopyable.hpp"
#include "../std/iostream.hpp"


class Track;

class BookmarkCustomData : public UserCustomData
{
public:
  BookmarkCustomData(string const & name, string const & type,
                     string const & description = "", double scale = -1.0,
                     time_t timeStamp = my::INVALID_TIME_STAMP)
    : m_name(name)
    , m_description(description)
    , m_type(type)
    , m_scale(scale)
    , m_timeStamp(timeStamp)
  {
  }

  virtual Type GetType() const { return BOOKMARK; }

  string const & GetName() const { return m_name; }
  void SetName(const string & name) { m_name = name; }

  string const & GetDescription() const { return m_description; }
  void SetDescription(const string & description) { m_description = description; }

  string const & GetTypeName() const { return m_type; }
  void SetTypeName(const string & type) { m_type = type; }

  double const & GetScale() const { return m_scale; }
  void SetScale(double scale) { m_scale = scale; }

  time_t const & GetTimeStamp() const { return m_timeStamp; }
  void SetTimeStamp(const time_t & timeStamp) { m_timeStamp = timeStamp; }

private:
  string m_name;
  string m_description;
  string m_type;  ///< Now it stores bookmark color (category style).
  double m_scale; ///< Viewport scale. -1.0 - is a default value (no scale set).
  time_t m_timeStamp;
};

class Bookmark : public ICustomDrawable
{
public:
  Bookmark(m2::PointD const & ptOrg, UserMarkContainer * container)
    : ICustomDrawable(ptOrg, container)
  {
    Inject();
  }

  m2::PointD const & GetOrg() const { return UserMark::GetOrg(); }
  string const & GetName() const { return GetData()->GetName(); }
  //void SetName(string const & name) { m_name = name; }
  /// @return Now its a bookmark color - name of icon file
  string const & GetType() const { return GetData()->GetTypeName(); }
  //void SetType(string const & type) { m_type = type; }
  m2::RectD GetViewport() const { return m2::RectD(GetOrg(), GetOrg()); }

  string const & GetDescription() const { return GetData()->GetDescription(); }
  void SetDescription(string const & description) { GetData()->SetDescription(description); }

  /// @return my::INVALID_TIME_STAMP if bookmark has no timestamp
  time_t GetTimeStamp() const { return GetData()->GetTimeStamp(); }
  void SetTimeStamp(time_t timeStamp) { GetData()->SetTimeStamp(timeStamp); }

  double GetScale() const { return GetData()->GetScale(); }
  void SetScale(double scale) { GetData()->SetScale(scale); }

  virtual graphics::DisplayList * GetDisplayList(UserMarkDLCache * cache) const
  {
    return cache->FindUserMark(UserMarkDLCache::Key(GetType(), graphics::EPosAbove, GetContainer()->GetDepth()));
  }

private:
  void Inject(m2::PointD const & org = m2::PointD(), string const & name = "",
              string const & type = "", string const & descr = "",
              double scale = -1, time_t timeStamp = my::INVALID_TIME_STAMP)
  {
    InjectCustomData(new BookmarkCustomData(name, descr, type, scale, timeStamp));
  }

  BookmarkCustomData * GetData()
  {
    return static_cast<BookmarkCustomData *>(&GetCustomData());
  }

  BookmarkCustomData const * GetData() const
  {
    return static_cast<BookmarkCustomData const *>(&GetCustomData());
  }
};

class BookmarkCategory : public UserMarkContainer
{
  typedef UserMarkContainer base_t;
  /// @name Data
  //@{
  /// TODO move track into UserMarkContainer as a IDrawable custom data
  vector<Track *> m_tracks;
  //@}

  string m_name;
  /// Stores file name from which category was loaded
  string m_file;

public:
  BookmarkCategory(string const & name, Framework & framework);
  ~BookmarkCategory();

  void ClearBookmarks();
  void ClearTracks();

  static string GetDefaultType();

  /// @name Theese functions are called from Framework only.
  //@{
  void AddBookmark(m2::PointD const & ptOrg, BookmarkCustomData const & bm);
  void ReplaceBookmark(size_t index, BookmarkCustomData const & bm);
  //@}

  /// @name Tracks routine.
  //@{
  /// @note Move semantics is used here.
  void AddTrack(Track & track);
  Track const * GetTrack(size_t index) const;
  inline size_t GetTracksCount() const { return m_tracks.size(); }
  void DeleteTrack(size_t index);
  //@}

  void SetName(string const & name) { m_name = name; }
  string const & GetName() const { return m_name; }
  string const & GetFileName() const { return m_file; }

  size_t GetBookmarksCount() const;

  Bookmark const * GetBookmark(size_t index) const;
  Bookmark * GetBookmark(size_t index);
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
  static BookmarkCategory * CreateFromKMLFile(string const & file, Framework & framework);

  /// Get valid file name from input (remove illegal symbols).
  static string RemoveInvalidSymbols(string const & name);
  /// Get unique bookmark file name from path and valid file name.
  static string GenerateUniqueFileName(const string & path, string name);
  //@}

protected:
  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg);
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
