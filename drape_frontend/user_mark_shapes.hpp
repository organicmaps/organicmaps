#pragma once

#include "tile_key.hpp"
#include "user_marks_provider.hpp"

#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/point2d.hpp"

#include "std/function.hpp"

namespace df
{

struct UserMarkRenderParams
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

struct UserLineRenderParams
{
  std::vector<LineLayer> m_layers;
  std::vector<m2::PointD> m_points;
};

using UserMarksRenderCollection = std::vector<UserMarkRenderParams>;
using UserLinesRenderCollection = std::vector<UserLineRenderParams>;

using MarkIndexesCollection = std::vector<uint32_t>;
using LineIndexesCollection = std::vector<uint32_t>;

struct UserMarkRenderData
{
  UserMarkRenderData(dp::GLState const & state,
                     drape_ptr<dp::RenderBucket> && bucket,
                     TileKey const & tileKey)
    : m_state(state), m_bucket(move(bucket)), m_tileKey(tileKey)
  {}

  dp::GLState m_state;
  drape_ptr<dp::RenderBucket> m_bucket;
  TileKey m_tileKey;
};

using TUserMarksRenderData = std::vector<UserMarkRenderData>;

class UserMarkShape
{
public:
  static void Draw(TileKey const & tileKey, ref_ptr<dp::TextureManager> textures,
                   UserMarksRenderCollection const & renderParams, MarkIndexesCollection const & indexes,
                   TUserMarksRenderData & renderData);
};

class UserLineShape
{
public:
  static void Draw(TileKey const & tileKey, ref_ptr<dp::TextureManager> textures, UserLinesRenderCollection const & renderParams,
                   LineIndexesCollection const & indexes, TUserMarksRenderData & renderData);
};

} // namespace df
