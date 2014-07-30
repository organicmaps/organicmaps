#include "engine_context.hpp"

#include "message_subclasses.hpp"
#include "map_shape.hpp"

namespace df
{

EngineContext::EngineContext(dp::RefPointer<ThreadsCommutator> commutator)
  : m_commutator(commutator)
{
}

void EngineContext::BeginReadTile(TileKey const & key)
{
  PostMessage(new TileReadStartMessage(key));
}

void EngineContext::InsertShape(TileKey const & key, dp::TransferPointer<MapShape> shape)
{
  PostMessage(new MapShapeReadedMessage(key, shape));
}

void EngineContext::EndReadTile(TileKey const & key)
{
  PostMessage(new TileReadEndMessage(key));
}

void EngineContext::PostMessage(Message * message)
{
  m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread, dp::MovePointer(message));
}

} // namespace df
