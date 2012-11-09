#include "../base/SRC_FIRST.hpp"

#include "tile.hpp"

#include "../graphics/base_texture.hpp"

Tile::Tile()
{}

Tile::Tile(shared_ptr<graphics::gl::BaseTexture> const & renderTarget,
           shared_ptr<graphics::Overlay> const & overlay,
           ScreenBase const & tileScreen,
           Tiler::RectInfo const & rectInfo,
           double duration,
           bool isEmptyDrawing)
  : m_renderTarget(renderTarget),
    m_overlay(overlay),
    m_tileScreen(tileScreen),
    m_rectInfo(rectInfo),
    m_duration(duration),
    m_isEmptyDrawing(isEmptyDrawing)
{}

Tile::~Tile()
{}

bool LessRectInfo::operator()(Tile const * l, Tile const * r) const
{
  return l->m_rectInfo < r->m_rectInfo;
}
