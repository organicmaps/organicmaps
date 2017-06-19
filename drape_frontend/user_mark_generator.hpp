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
struct UserMarkRenderInfo
{
  m2::PointD m_pivot;
  m2::PointD m_pixelOffset;
  std::string m_symbolName;
  dp::Anchor m_anchor;
  float m_depth;
  bool m_runCreationAnim;
  bool m_isVisible;
};

struct LineLayer
{
  dp::Color m_color;
  float m_width;
  float m_depth;
};

struct UserLineRenderInfo
{
  std::vector<LineLayer> m_layers;
  std::vector<m2::PointD> m_points;
};

using UserMarksRenderCollection = std::vector<UserMarkRenderInfo>;
using UserLinesRenderCollection = std::vector<UserLineRenderInfo>;

using GroupID = uint32_t;

using MarkGroups = std::map<GroupID, drape_ptr<UserMarksRenderCollection>>;
using LineGroups = std::map<GroupID, drape_ptr<UserLinesRenderCollection>>;

using MarkIndexesCollection = std::vector<uint32_t>;
using MarkIndexesGroups = std::map<GroupID, drape_ptr<MarkIndexesCollection>>;

using MarksIndex = std::map<TileKey, drape_ptr<MarkIndexesGroups>>;

class UserMarkGenerator
{
public:
  using TFlushFn = function<void(TUserMarkShapes && shapes)>;

  UserMarkGenerator(TFlushFn const & flushFn);

  void SetUserMarks(GroupID groupId, drape_ptr<UserMarksRenderCollection> && marks);
  void SetUserLines(GroupID groupId, drape_ptr<UserLinesRenderCollection> && lines);

  void SetGroupVisibility(GroupID groupId, bool isVisible);

  void GenerateUserMarksGeometry(TileKey const & tileKey, ref_ptr<dp::TextureManager> textures);

private:
  TFlushFn m_flushFn;
private:
  void UpdateMarksIndex(GroupID groupId);
  void UpdateLinesIndex(GroupID groupId);

  struct UserMarkBatcherKey
  {
    UserMarkBatcherKey() = default;
    UserMarkBatcherKey(TileKey const & tileKey, GroupID groupId)
      : m_tileKey(tileKey)
      , m_groupId(groupId)
    {}
    GroupID m_groupId;
    TileKey m_tileKey;
  };

  struct UserMarkBatcherKeyComparator
  {
    bool operator() (UserMarkBatcherKey const & lhs, UserMarkBatcherKey const & rhs) const
    {
      if (lhs.m_groupId == rhs.m_groupId)
        return lhs.m_tileKey.LessStrict(rhs.m_tileKey);
      return lhs.m_groupId < rhs.m_groupId;
    }
  };

  void FlushGeometry(UserMarkBatcherKey const & key, dp::GLState const & state,
                     drape_ptr<dp::RenderBucket> && buffer);

  drape_ptr<BatchersPool<UserMarkBatcherKey, UserMarkBatcherKeyComparator>> m_batchersPool;
  TFlushFn m_flushRenderDataFn;

  std::map<GroupID, bool> m_groupsVisibility;

  MarkGroups m_marks;
  LineGroups m_lines;

  MarksIndex m_marksIndex;
};

}  // namespace df
