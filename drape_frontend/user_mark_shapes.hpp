#pragma once

#include "tile_key.hpp"
#include "user_marks_provider.hpp"

#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/point2d.hpp"

#include "std/function.hpp"

namespace df
{

struct UserMarkShape
{
  dp::GLState m_state;
  drape_ptr<dp::RenderBucket> m_bucket;
  TileKey m_tileKey;

  UserMarkShape(dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket,
                TileKey const & tileKey)
    : m_state(state), m_bucket(move(bucket)), m_tileKey(tileKey)
  {}
};

using TUserMarkShapes = vector<UserMarkShape>;

TUserMarkShapes CacheUserMarks(UserMarksProvider const * provider,
                               ref_ptr<dp::TextureManager> textures);

} // namespace df
