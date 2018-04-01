#pragma once

#include "map/track.hpp"
#include "map/user_mark.hpp"
#include "map/user_mark_layer.hpp"

#include "kml/types.hpp"

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

class Bookmark : public UserMark
{
  using Base = UserMark;
public:
  Bookmark(m2::PointD const & ptOrg);

  Bookmark(kml::BookmarkData const & data);

  void SetData(kml::BookmarkData const & data);
  kml::BookmarkData const & GetData() const;

  bool HasCreationAnimation() const override;

  std::string GetName() const;
  void SetName(std::string const & name);

  kml::PredefinedColor GetColor() const;
  void SetColor(kml::PredefinedColor color);

  m2::RectD GetViewport() const;

  std::string GetDescription() const;
  void SetDescription(std::string const & description);

  kml::Timestamp GetTimeStamp() const;
  void SetTimeStamp(kml::Timestamp timeStamp);

  uint8_t GetScale() const;
  void SetScale(uint8_t scale);

  dp::Anchor GetAnchor() const override;
  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;

  df::ColorConstant GetColorConstant() const override;

  df::MarkGroupID GetGroupId() const override;

  void Attach(df::MarkGroupID groupId);
  void Detach();

private:
  kml::BookmarkData m_data;
  df::MarkGroupID m_groupId;
};

class BookmarkCategory : public UserMarkLayer
{
  using Base = UserMarkLayer;

public:
  BookmarkCategory(std::string const & name, df::MarkGroupID groupId, bool autoSave);
  BookmarkCategory(kml::CategoryData const & data, df::MarkGroupID groupId, bool autoSave);
  ~BookmarkCategory() override;

  static kml::PredefinedColor GetDefaultColor();

  df::MarkGroupID GetID() const { return m_groupId; }

  void SetIsVisible(bool isVisible) override;
  void SetName(std::string const & name);
  void SetFileName(std::string const & fileName) { m_file = fileName; }
  std::string GetName() const;
  std::string const & GetFileName() const { return m_file; }

  void EnableAutoSave(bool enable) { m_autoSave = enable; }
  bool IsAutoSaveEnabled() const { return m_autoSave; }

  kml::CategoryData const & GetCategoryData() const { return m_data; }

private:
  df::MarkGroupID const m_groupId;
  // Stores file name from which bookmarks were loaded.
  std::string m_file;
  bool m_autoSave = true;
  kml::CategoryData m_data;
};

std::unique_ptr<kml::FileData> LoadKMLFile(std::string const & file, bool useBinary);
std::unique_ptr<kml::FileData> LoadKMLData(Reader const & reader, bool useBinary);
