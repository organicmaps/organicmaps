#pragma once

#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "drape/pointers.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace df
{
using MarksIDGroups = std::map<MarkGroupID, drape_ptr<IDCollections>>;
using MarksIndex = std::map<TileKey, drape_ptr<MarksIDGroups>>;

class UserMarkGenerator
{
public:
  using TFlushFn = function<void(TUserMarksRenderData && renderData)>;

  UserMarkGenerator(TFlushFn const & flushFn);

  void SetUserMarks(drape_ptr<UserMarksRenderCollection> && marks);
  void SetUserLines(drape_ptr<UserLinesRenderCollection> && lines);
  void SetRemovedUserMarks(drape_ptr<IDCollections> && ids);
  void SetCreatedUserMarks(drape_ptr<IDCollections> && ids);

  void SetGroup(MarkGroupID groupId, drape_ptr<IDCollections> && ids);
  void RemoveGroup(MarkGroupID groupId);
  void SetGroupVisibility(MarkGroupID groupId, bool isVisible);

  void GenerateUserMarksGeometry(TileKey const & tileKey, ref_ptr<dp::TextureManager> textures);

private:
  void UpdateIndex(MarkGroupID groupId);

  ref_ptr<IDCollections> GetIdCollection(TileKey const & tileKey, MarkGroupID groupId);
  void CleanIndex();

  int GetNearestLineIndexZoom(int zoom) const;

  ref_ptr<MarksIDGroups> GetUserMarksGroups(TileKey const & tileKey);
  ref_ptr<MarksIDGroups> GetUserLinesGroups(TileKey const & tileKey);

  void CacheUserMarks(TileKey const & tileKey, MarksIDGroups const & indexesGroups,
                      ref_ptr<dp::TextureManager> textures, dp::Batcher & batcher);
  void CacheUserLines(TileKey const & tileKey, MarksIDGroups const & indexesGroups,
                      ref_ptr<dp::TextureManager> textures, dp::Batcher & batcher);

  std::unordered_set<MarkGroupID> m_groupsVisibility;
  MarksIDGroups m_groups;

  UserMarksRenderCollection m_marks;
  UserLinesRenderCollection m_lines;

  MarksIndex m_index;

  TFlushFn m_flushFn;
};
}  // namespace df
