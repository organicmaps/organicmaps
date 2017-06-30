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
using GroupID = size_t;

using MarksIDGroups = std::map<GroupID, drape_ptr<IDCollection>>;
using MarksIndex = std::map<TileKey, drape_ptr<MarksIDGroups>>;

class UserMarkGenerator
{
public:
  using TFlushFn = function<void(TUserMarksRenderData && renderData)>;

  UserMarkGenerator(TFlushFn const & flushFn);

  void SetUserMarks(drape_ptr<UserMarksRenderCollection> && marks);
  void SetUserLines(drape_ptr<UserLinesRenderCollection> && lines);

  void SetGroup(GroupID groupId, drape_ptr<IDCollection> && ids);
  void RemoveGroup(GroupID groupId);
  void RemoveUserMarks(IDCollection && ids);

  void SetGroupVisibility(GroupID groupId, bool isVisible);

  void GenerateUserMarksGeometry(TileKey const & tileKey, ref_ptr<dp::TextureManager> textures);

private:
  void UpdateIndex(GroupID groupId);

  ref_ptr<IDCollection> GetIdCollection(TileKey const & tileKey, GroupID groupId);
  void CleanIndex();

  std::unordered_set<GroupID> m_groupsVisibility;
  MarksIDGroups m_groups;

  UserMarksRenderCollection m_marks;
  UserLinesRenderCollection m_lines;

  MarksIndex m_index;

  TFlushFn m_flushFn;
};
}  // namespace df
