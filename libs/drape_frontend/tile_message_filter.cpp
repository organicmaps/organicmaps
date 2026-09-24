#include "drape_frontend/tile_message_filter.hpp"

#include "drape_frontend/message_subclasses.hpp"

#include <vector>

namespace df
{
bool FilterTileMessage(ref_ptr<Message> message, std::function<bool(TileKey const &)> const & discard)
{
  switch (message->GetType())
  {
  case Message::Type::FlushTile: return discard(ref_ptr<FlushRenderBucketMessage>(message)->GetKey());
  case Message::Type::FlushTrafficData:
    return discard(ref_ptr<FlushTrafficDataMessage>(message)->AcceptRenderData().m_tileKey);
  case Message::Type::FlushOverlays:
  {
    auto && data = ref_ptr<FlushOverlaysMessage>(message)->AcceptRenderData();
    std::erase_if(data, [&](auto const & entry) { return discard(entry.m_tileKey); });
    break;
  }
  case Message::Type::FlushUserMarks:
  {
    auto && data = ref_ptr<FlushUserMarksMessage>(message)->AcceptRenderData();
    std::erase_if(data, [&](auto const & entry) { return discard(entry.m_tileKey); });
    break;
  }
  default: break;
  }
  return false;
}
}  // namespace df
