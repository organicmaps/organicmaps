#pragma once

#include "map/track.hpp"
#include "map/user_mark.hpp"
#include "map/user_mark_layer.hpp"

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

class BookmarkManager;

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
  using Base = UserMark;
public:
  Bookmark(m2::PointD const & ptOrg);

  Bookmark(BookmarkData const & data, m2::PointD const & ptOrg);

  void SetData(BookmarkData const & data);
  BookmarkData const & GetData() const;

  dp::Anchor GetAnchor() const override;
  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  bool HasCreationAnimation() const override;

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

  df::MarkGroupID GetGroupId() const override;
  void Attach(df::MarkGroupID groupId);
  void Detach();

private:
  BookmarkData m_data;
  df::MarkGroupID m_groupId;
};

class BookmarkCategory : public UserMarkLayer
{
  using Base = UserMarkLayer;

public:
  BookmarkCategory(std::string const & name, df::MarkGroupID groupID);
  ~BookmarkCategory() override;

  static std::string GetDefaultType();

  void AttachTrack(df::LineID markId);
  void DetachTrack(df::LineID markId);

  df::MarkGroupID GetID() const { return m_groupId; }
  df::LineIDSet const & GetUserLines() const override { return m_tracks; }

  void SetName(std::string const & name) { m_name = name; }
  void SetFileName(std::string const & fileName) { m_file = fileName; }
  std::string const & GetName() const { return m_name; }
  std::string const & GetFileName() const { return m_file; }

private:
  df::MarkGroupID const m_groupId;
  std::string m_name;
  // Stores file name from which bookmarks were loaded.
  std::string m_file;

  df::LineIDSet m_tracks;
};

struct KMLData
{
  std::string m_name;
  std::string m_file;
  std::vector<std::unique_ptr<Bookmark>> m_bookmarks;
  std::vector<std::unique_ptr<Track>> m_tracks;
  bool m_visible = true;
};

std::unique_ptr<KMLData> LoadKMLFile(std::string const & file);
