#pragma once

#include "map/user_mark.hpp"
#include "map/user_mark_container.hpp"

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

class Bookmark : public UserMark
{
  using TBase = UserMark;
public:
  Bookmark(m2::PointD const & ptOrg, UserMarkContainer * container);

  Bookmark(BookmarkData const & data, m2::PointD const & ptOrg,
           UserMarkContainer * container);

  void SetData(BookmarkData const & data);
  BookmarkData const & GetData() const;

  dp::Anchor GetAnchor() const override;
  string GetSymbolName() const override;

  Type GetMarkType() const override;
  bool RunCreationAnim() const override;

  string const & GetName() const;
  void SetName(string const & name);
  /// @return Now its a bookmark color - name of icon file
  string const & GetType() const;
  void SetType(string const & type);
  m2::RectD GetViewport() const;

  string const & GetDescription() const;
  void SetDescription(string const & description);

  /// @return my::INVALID_TIME_STAMP if bookmark has no timestamp
  time_t GetTimeStamp() const;
  void SetTimeStamp(time_t timeStamp);

  double GetScale() const;
  void SetScale(double scale);

private:
  BookmarkData m_data;
  mutable bool m_runCreationAnim;
};

class BookmarkCategory : public UserMarkContainer
{
  typedef UserMarkContainer TBase;
  vector<unique_ptr<Track>> m_tracks;

  string m_name;
  /// Stores file name from which category was loaded
  string m_file;

public:
  class Guard
  {
  public:
    Guard(BookmarkCategory & cat)
      : m_controller(cat.RequestController())
      , m_cat(cat)
    {
    }

    ~Guard()
    {
      m_cat.ReleaseController();
    }

    UserMarksController & m_controller;

  private:
    BookmarkCategory & m_cat;
  };

  BookmarkCategory(string const & name, Framework & framework);
  ~BookmarkCategory() override;

  size_t GetUserLineCount() const override;
  df::UserLineMark const * GetUserLineMark(size_t index) const override;

  static string GetDefaultType();

  void ClearTracks();

  /// @name Tracks routine.
  //@{
  void AddTrack(unique_ptr<Track> && track);
  Track const * GetTrack(size_t index) const;
  inline size_t GetTracksCount() const { return m_tracks.size(); }
  void DeleteTrack(size_t index);
  //@}

  void SetName(string const & name) { m_name = name; }
  string const & GetName() const { return m_name; }
  string const & GetFileName() const { return m_file; }

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
  UserMark * AllocateUserMark(m2::PointD const & ptOrg) override;
};

struct BookmarkAndCategory
{
  BookmarkAndCategory() = default;
  BookmarkAndCategory(size_t bookmarkIndex, size_t categoryIndex) : m_bookmarkIndex(bookmarkIndex),
                                                              m_categoryIndex(categoryIndex) {}

  bool IsValid() const
  {
    return m_bookmarkIndex != numeric_limits<size_t>::max() &&
                          m_categoryIndex != numeric_limits<size_t>::max();
  };

  size_t m_bookmarkIndex = numeric_limits<size_t>::max();
  size_t m_categoryIndex = numeric_limits<size_t>::max();
};
