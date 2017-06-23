#pragma once

#include "drape_frontend/batchers_pool.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"

#include "indexer/scales.hpp"

#include <array>
#include <map>
#include <string>
#include <vector>

namespace df
{

using GroupID = uint32_t;
using MarkGroups = std::map<GroupID, drape_ptr<UserMarksRenderCollection>>;
using LineGroups = std::map<GroupID, drape_ptr<UserLinesRenderCollection>>;

struct IndexesCollection
{
  MarkIndexesCollection m_markIndexes;
  LineIndexesCollection m_lineIndexes;
};

using MarkIndexesGroups = std::map<GroupID, drape_ptr<IndexesCollection>>;
using MarksIndex = std::map<TileKey, drape_ptr<MarkIndexesGroups>>;

class UserMarkGenerator
{
public:
  using TFlushFn = function<void(GroupID, TUserMarksRenderData && renderData)>;

  UserMarkGenerator(TFlushFn const & flushFn);

  void SetUserMarks(GroupID groupId, drape_ptr<UserMarksRenderCollection> && marks);
  void SetUserLines(GroupID groupId, drape_ptr<UserLinesRenderCollection> && lines);

  void SetGroupVisibility(GroupID groupId, bool isVisible);

  void GenerateUserMarksGeometry(TileKey const & tileKey, ref_ptr<dp::TextureManager> textures);

private:
  void UpdateMarksIndex(GroupID groupId);
  void UpdateLinesIndex(GroupID groupId);

  ref_ptr<IndexesCollection> GetIndexesCollection(TileKey const & tileKey, GroupID groupId);
  void CleanIndex();

  std::map<GroupID, bool> m_groupsVisibility;

  MarkGroups m_marks;
  LineGroups m_lines;

  MarksIndex m_marksIndex;

  TFlushFn m_flushFn;
};

}  // namespace df
