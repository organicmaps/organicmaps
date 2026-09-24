#pragma once

#include "drape_frontend/tile_key.hpp"

#include "drape/pointers.hpp"

#include <functional>

namespace df
{
class Message;

// Returns true when the whole message can be removed. Batched payloads are filtered in place;
// their messages and all completion messages survive to preserve frontend notifications.
bool FilterTileMessage(ref_ptr<Message> message, std::function<bool(TileKey const &)> const & discard);
}  // namespace df
