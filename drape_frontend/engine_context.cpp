#include "drape_frontend/engine_context.hpp"

//#define DRAW_TILE_NET

#include "drape_frontend/message_subclasses.hpp"
#include "drape/texture_manager.hpp"

#ifdef DRAW_TILE_NET
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/text_shape.hpp"

#include "base/string_utils.hpp"
#endif

namespace df
{

EngineContext::EngineContext(TileKey tileKey, ref_ptr<ThreadsCommutator> commutator)
  : m_tileKey(tileKey)
  , m_commutator(commutator)
{
}

void EngineContext::BeginReadTile()
{
  PostMessage(make_unique_dp<TileReadStartMessage>(m_tileKey));
}

void EngineContext::Flush(list<drape_ptr<MapShape>> && shapes)
{
  PostMessage(make_unique_dp<MapShapeReadedMessage>(m_tileKey, move(shapes)));
}

void EngineContext::EndReadTile(ref_ptr<dp::TextureManager> texMng)
{
#ifdef DRAW_TILE_NET
  m2::RectD r = m_tileKey.GetGlobalRect();
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

  InsertShape(make_unique_dp<LineShape>(spline, p, texMng));

  df::TextViewParams tp;
  tp.m_anchor = dp::Center;
  tp.m_depth = 0;
  tp.m_primaryText = strings::to_string(m_tileKey.m_x) + " " +
                     strings::to_string(m_tileKey.m_y) + " " +
                     strings::to_string(m_tileKey.m_zoomLevel);

  tp.m_primaryTextFont = dp::FontDecl(dp::Color::Red(), 30);

  list<drape_ptr<MapShape>> shapes;
  shapes.push_back(make_unique_dp<TextShape>(r.Center(), tp));

  Flush(move(shapes));
#endif

  PostMessage(make_unique_dp<TileReadEndMessage>(m_tileKey));
}

void EngineContext::PostMessage(drape_ptr<Message> && message)
{
  m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread, move(message),
                            MessagePriority::Normal);
}

} // namespace df
