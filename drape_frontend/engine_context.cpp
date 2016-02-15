#include "drape_frontend/engine_context.hpp"

#include "drape_frontend/message_subclasses.hpp"
#include "drape/texture_manager.hpp"

#include "std/algorithm.hpp"

namespace df
{

EngineContext::EngineContext(TileKey tileKey, ref_ptr<ThreadsCommutator> commutator,
                             ref_ptr<dp::TextureManager> texMng)
  : m_tileKey(tileKey)
  , m_commutator(commutator)
  , m_texMng(texMng)
{
  int const kAverageShapesCount = 300;
  m_overlayShapes.reserve(kAverageShapesCount);
}

ref_ptr<dp::TextureManager> EngineContext::GetTextureManager() const
{
  return m_texMng;
}

void EngineContext::BeginReadTile()
{
  PostMessage(make_unique_dp<TileReadStartMessage>(m_tileKey));
}

void EngineContext::Flush(TMapShapes && shapes)
{
  PostMessage(make_unique_dp<MapShapeReadedMessage>(m_tileKey, move(shapes)));
}

void EngineContext::FlushOverlays(TMapShapes && shapes)
{
  lock_guard<mutex> lock(m_overlayShapesMutex);
  m_overlayShapes.reserve(m_overlayShapes.size() + shapes.size());
  move(shapes.begin(), shapes.end(), back_inserter(m_overlayShapes));
}

void EngineContext::EndReadTile()
{
  lock_guard<mutex> lock(m_overlayShapesMutex);
  if (!m_overlayShapes.empty())
  {
    TMapShapes overlayShapes;
    overlayShapes.swap(m_overlayShapes);
    PostMessage(make_unique_dp<OverlayMapShapeReadedMessage>(m_tileKey, move(overlayShapes)));
  }

  PostMessage(make_unique_dp<TileReadEndMessage>(m_tileKey));
}

void EngineContext::PostMessage(drape_ptr<Message> && message)
{
  m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread, move(message),
                            MessagePriority::Normal);
}

} // namespace df
