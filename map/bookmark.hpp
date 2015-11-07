#pragma once

#include "map/user_mark.hpp"
#include "map/user_mark_container.hpp"
#include "map/styled_point.hpp"

#include "coding/reader.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/timer.hpp"

#include "std/string.hpp"
#include "std/noncopyable.hpp"
#include "std/iostream.hpp"
#include "std/shared_ptr.hpp"

namespace anim
{
  class Task;
}

class Track;

class BookmarkData
{
public:
  BookmarkData()
    : m_scale(-1.0)
    , m_timeStamp(my::INVALID_TIME_STAMP)
  {
  }

  BookmarkData(string const & name, string const & type,
                     string const & description = "", double scale = -1.0,
                     time_t timeStamp = my::INVALID_TIME_STAMP)
    : m_name(name)
    , m_description(description)
    , m_type(type)
    , m_scale(scale)
    , m_timeStamp(timeStamp)
  {
  }

  string const & GetName() const { return m_name; }
  void SetName(const string & name) { m_name = name; }

  string const & GetDescription() const { return m_description; }
  void SetDescription(const string & description) { m_description = description; }

  string const & GetType() const { return m_type; }
  void SetType(const string & type) { m_type = type; }

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

class Bookmark : public StyledPoint
{
  BookmarkData m_data;
  double m_animScaleFactor;

public:
  Bookmark(m2::PointD const & ptOrg, UserMarkContainer * container)
    : StyledPoint(ptOrg, container), m_animScaleFactor(1.0)
  {
  }

  Bookmark(BookmarkData const & data, m2::PointD const & ptOrg, UserMarkContainer * container)
    : StyledPoint(data.GetType(), ptOrg, container), m_data(data), m_animScaleFactor(1.0)
  {
  }

  void SetData(BookmarkData const & data)
  {
    m_data = data;
    SetStyle(m_data.GetType());
  }

  BookmarkData const & GetData() const { return m_data; }

  virtual Type GetMarkType() const override { return UserMark::Type::BOOKMARK; }
  virtual void FillLogEvent(TEventContainer & details) const override;

  string const & GetName() const { return m_data.GetName(); }
  void SetName(string const & name) { m_data.SetName(name); }
  /// @return Now its a bookmark color - name of icon file
  string const & GetType() const { return m_data.GetType(); }

  void SetType(string const & type)
  {
    m_data.SetType(type);
    SetStyle(type);
  }

  m2::RectD GetViewport() const { return m2::RectD(GetOrg(), GetOrg()); }

  string const & GetDescription() const { return m_data.GetDescription(); }
  void SetDescription(string const & description) { m_data.SetDescription(description); }

  /// @return my::INVALID_TIME_STAMP if bookmark has no timestamp
  time_t GetTimeStamp() const { return m_data.GetTimeStamp(); }
  void SetTimeStamp(time_t timeStamp) { m_data.SetTimeStamp(timeStamp); }

  double GetScale() const { return m_data.GetScale(); }
  void SetScale(double scale) { m_data.SetScale(scale); }

  unique_ptr<UserMarkCopy> Copy() const override;

  shared_ptr<anim::Task> CreateAnimTask(Framework & fm);
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

  virtual Type GetType() const { return BOOKMARK_MARK; }

  void ClearBookmarks();
  void ClearTracks();

  static string GetDefaultType();

  /// @name Theese functions are called from Framework only.
  //@{
  Bookmark* AddBookmark(m2::PointD const & ptOrg, BookmarkData const & bm);
  void ReplaceBookmark(size_t index, BookmarkData const & bm);
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

  // Returns index of the bookmark if exists, otherwise returns
  // total number of bookmarks.
  size_t FindBookmark(Bookmark const * bookmark) const;

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
  virtual string GetTypeName() const { return "search-result"; }
  virtual string GetActiveTypeName() const { return "search-result-active"; }
  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg);

private:
  void ReleaseAnimations();
  
private:
  bool m_blockAnimation;
  typedef pair<UserMark *, shared_ptr<anim::Task> > anim_node_t;
  vector<anim_node_t> m_anims;
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
