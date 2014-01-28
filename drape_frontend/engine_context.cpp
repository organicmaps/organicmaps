#include "engine_context.hpp"

#include "message_subclasses.hpp"
#include "map_shape.hpp"

namespace df
{
  EngineContext::EngineContext(RefPointer<ThreadsCommutator> commutator, ScalesProcessor const & processor)
    : m_commutator(commutator)
    , m_scalesProcessor(processor)
  {
  }

  ScalesProcessor const & EngineContext::GetScalesProcessor() const
  {
    return m_scalesProcessor;
  }

  void EngineContext::BeginReadTile(TileKey const & key)
  {
    PostMessage(new TileReadStartMessage(key));
  }

  void EngineContext::InsertShape(TileKey const & key, TransferPointer<MapShape> shape)
  {
    PostMessage(new MapShapeReadedMessage(key, shape));
  }

  void EngineContext::EndReadTile(TileKey const & key)
  {
    PostMessage(new TileReadEndMessage(key));
  }

  void EngineContext::PostMessage(Message * message)
  {
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread, MovePointer(message));
  }
}
