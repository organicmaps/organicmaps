#pragma once

#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "drape/pointers.hpp"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace df
{
using MarksIDGroups = std::map<kml::MarkGroupId, drape_ptr<IDCollections>>;
using MarksIndex = std::map<TileKey, drape_ptr<MarksIDGroups>>;

class UserMarkGenerator
{
public:
  using TFlushFn = std::function<void(TUserMarksRenderData && renderData)>;

  explicit UserMarkGenerator(TFlushFn const & flushFn);

  void SetUserMarks(drape_ptr<UserMarksRenderCollection> && marks);
  void SetUserLines(drape_ptr<UserLinesRenderCollection> && lines);
  void SetRemovedUserMarks(drape_ptr<IDCollections> && ids);
  void SetJustCreatedUserMarks(drape_ptr<IDCollections> && ids);

  void SetGroup(kml::MarkGroupId groupId, drape_ptr<IDCollections> && ids);
  void RemoveGroup(kml::MarkGroupId groupId);
  void SetGroupVisibility(kml::MarkGroupId groupId, bool isVisible);

  void GenerateUserMarksGeometry(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                                 ref_ptr<dp::TextureManager> textures);

private:
  void UpdateIndex(kml::MarkGroupId groupId);

  ref_ptr<IDCollections> GetIdCollection(TileKey const & tileKey, kml::MarkGroupId groupId);
  void CleanIndex();

  int GetNearestLineIndexZoom(int zoom) const;

  ref_ptr<MarksIDGroups> GetUserMarksGroups(TileKey const & tileKey);
  ref_ptr<MarksIDGroups> GetUserLinesGroups(TileKey const & tileKey);

  void CacheUserMarks(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                      MarksIDGroups const & indexesGroups, ref_ptr<dp::TextureManager> textures,
                      dp::Batcher & batcher) const;
  void CacheUserLines(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                      MarksIDGroups const & indexesGroups, ref_ptr<dp::TextureManager> textures,
                      dp::Batcher & batcher) const;

  std::unordered_set<kml::MarkGroupId> m_groupsVisibility;
  MarksIDGroups m_groups;

  UserMarksRenderCollection m_marks;
  UserLinesRenderCollection m_lines;

  MarksIndex m_index;

  TFlushFn m_flushFn;
};
}  // namespace df
