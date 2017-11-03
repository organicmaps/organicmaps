#pragma once

#include "map/user_mark.hpp"
#include "map/user_mark_container.hpp"

#include "coding/reader.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/timer.hpp"

#include "std/noncopyable.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

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

  BookmarkData(std::string const & name,
               std::string const & type,
               std::string const & description = "",
               double scale = -1.0,
               time_t timeStamp = my::INVALID_TIME_STAMP)
    : m_name(name)
    , m_description(description)
    , m_type(type)
    , m_scale(scale)
    , m_timeStamp(timeStamp)
  {
  }

  std::string const & GetName() const { return m_name; }
  void SetName(const std::string & name) { m_name = name; }

  std::string const & GetDescription() const { return m_description; }
  void SetDescription(const std::string & description) { m_description = description; }

  std::string const & GetType() const { return m_type; }
  void SetType(const std::string & type) { m_type = type; }

  double const & GetScale() const { return m_scale; }
  void SetScale(double scale) { m_scale = scale; }

  time_t const & GetTimeStamp() const { return m_timeStamp; }
  void SetTimeStamp(const time_t & timeStamp) { m_timeStamp = timeStamp; }

private:
  std::string m_name;
  std::string m_description;
  std::string m_type;  ///< Now it stores bookmark color (category style).
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
  std::string GetSymbolName() const override;
  bool HasCreationAnimation() const override;

  Type GetMarkType() const override;

  std::string const & GetName() const;
  void SetName(std::string const & name);
  /// @return Now its a bookmark color - name of icon file
  std::string const & GetType() const;
  void SetType(std::string const & type);
  m2::RectD GetViewport() const;

  std::string const & GetDescription() const;
  void SetDescription(std::string const & description);

  /// @return my::INVALID_TIME_STAMP if bookmark has no timestamp
  time_t GetTimeStamp() const;
  void SetTimeStamp(time_t timeStamp);

  double GetScale() const;
  void SetScale(double scale);

private:
  BookmarkData m_data;
};

class BookmarkCategory : public UserMarkContainer
{
  typedef UserMarkContainer TBase;
  std::vector<std::unique_ptr<Track>> m_tracks;

  std::string m_name;
  /// Stores file name from which category was loaded
  std::string m_file;

public:
  explicit BookmarkCategory(std::string const & name);
  ~BookmarkCategory() override;

  size_t GetUserLineCount() const override;
  df::UserLineMark const * GetUserLineMark(size_t index) const override;

  static std::string GetDefaultType();

  void ClearTracks();

  /// @name Tracks routine.
  //@{
  void AddTrack(std::unique_ptr<Track> && track);
  Track const * GetTrack(size_t index) const;
  inline size_t GetTracksCount() const { return m_tracks.size(); }
  void DeleteTrack(size_t index);
  //@}

  void SetName(std::string const & name) { m_name = name; }
  std::string const & GetName() const { return m_name; }
  std::string const & GetFileName() const { return m_file; }

  /// @name Theese fuctions are public for unit tests only.
  /// You don't need to call them from client code.
  //@{
  bool LoadFromKML(ReaderPtr<Reader> const & reader);
  void SaveToKML(std::ostream & s);

  /// Uses the same file name from which was loaded, or
  /// creates unique file name on first save and uses it every time.
  bool SaveToKMLFile();

  /// @return nullptr in the case of error
  static std::unique_ptr<BookmarkCategory> CreateFromKMLFile(std::string const & file);

  /// Get valid file name from input (remove illegal symbols).
  static std::string RemoveInvalidSymbols(std::string const & name);
  /// Get unique bookmark file name from path and valid file name.
  static std::string GenerateUniqueFileName(const std::string & path, std::string name);
  //@}

protected:
  UserMark * AllocateUserMark(m2::PointD const & ptOrg) override;
};

struct BookmarkAndCategory
{
  BookmarkAndCategory() = default;
  BookmarkAndCategory(size_t bookmarkIndex, size_t categoryIndex)
    : m_bookmarkIndex(bookmarkIndex)
    , m_categoryIndex(categoryIndex)
  {}

  bool IsValid() const
  {
    return m_bookmarkIndex != numeric_limits<size_t>::max() &&
           m_categoryIndex != numeric_limits<size_t>::max();
  };

  size_t m_bookmarkIndex = numeric_limits<size_t>::max();
  size_t m_categoryIndex = numeric_limits<size_t>::max();
};
