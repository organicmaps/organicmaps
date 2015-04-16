#pragma once

#include "tile_key.hpp"
#include "user_marks_provider.hpp"

#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/point2d.hpp"

#include "std/function.hpp"

namespace df
{
  TileKey GetSearchTileKey();
  TileKey GetApiTileKey();
  TileKey GetBookmarkTileKey(size_t categoryIndex);
  bool IsUserMarkLayer(TileKey const & tileKey);

  void CacheUserMarks(UserMarksProvider const * provider, ref_ptr<dp::Batcher> batcher,
                      ref_ptr<dp::TextureManager> textures);
} // namespace df
