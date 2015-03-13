#include "drape_frontend/engine_context.hpp"

//#define DRAW_TILE_NET

#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/map_shape.hpp"
#ifdef DRAW_TILE_NET
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/text_shape.hpp"

#include "base/string_utils.hpp"
#endif

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
#ifdef DRAW_TILE_NET
  m2::RectD r = key.GetGlobalRect();
  vector<m2::PointD> path;
  path.push_back(r.LeftBottom());
  path.push_back(r.LeftTop());
  path.push_back(r.RightTop());
  path.push_back(r.RightBottom());
  path.push_back(r.LeftBottom());

  m2::SharedSpline spline(path);
  df::LineViewParams p;
  p.m_baseGtoPScale = 1.0;
  p.m_cap = dp::ButtCap;
  p.m_color = dp::Color::Red();
  p.m_depth = 20000;
  p.m_width = 5;
  p.m_join = dp::RoundJoin;

  InsertShape(key, dp::MovePointer<df::MapShape>(new LineShape(spline, p)));

  df::TextViewParams tp;
  tp.m_anchor = dp::Center;
  tp.m_depth = 20000;
  tp.m_primaryText = strings::to_string(key.m_x) + " " +
                     strings::to_string(key.m_y) + " " +
                     strings::to_string(key.m_zoomLevel);

  tp.m_primaryTextFont = df::FontDecl(dp::Color::Red(), 30);

  InsertShape(key, dp::MovePointer<df::MapShape>(new TextShape(r.Center(), tp)));
#endif

  PostMessage(new TileReadEndMessage(key));
}

void EngineContext::PostMessage(Message * message)
{
  m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                            dp::MovePointer(message),
                            MessagePriority::Normal);
}

} // namespace df
