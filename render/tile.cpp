#include "tile.hpp"

#include "graphics/opengl/base_texture.hpp"

Tile::Tile()
{}

Tile::Tile(shared_ptr<graphics::gl::BaseTexture> const & renderTarget,
           const shared_ptr<graphics::OverlayStorage> & overlay,
           ScreenBase const & tileScreen,
           Tiler::RectInfo const & rectInfo,
           bool isEmptyDrawing,
           int sequenceID)
  : m_renderTarget(renderTarget),
    m_overlay(overlay),
    m_tileScreen(tileScreen),
    m_rectInfo(rectInfo),
    m_isEmptyDrawing(isEmptyDrawing),
    m_sequenceID(sequenceID)
{}

Tile::~Tile()
{}

bool LessRectInfo::operator()(Tile const * l, Tile const * r) const
{
  return l->m_rectInfo < r->m_rectInfo;
}
