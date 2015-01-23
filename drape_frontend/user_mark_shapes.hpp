#pragma once

#include "tile_key.hpp"
#include "user_marks_provider.hpp"

#include "../drape/batcher.hpp"
#include "../drape/texture_manager.hpp"

#include "../geometry/point2d.hpp"

#include "../std/function.hpp"

namespace df
{
  TileKey GetSearchTileKey();
  TileKey GetApiTileKey();
  TileKey GetBookmarkTileKey(size_t categoryIndex);
  bool IsUserMarkLayer(const TileKey & tileKey);

  void CacheUserMarks(UserMarksProvider const * provider, dp::RefPointer<dp::Batcher> batcher,
                      dp::RefPointer<dp::TextureManager> textures);
} // namespace df
