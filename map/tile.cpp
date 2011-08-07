#include "../base/SRC_FIRST.hpp"

#include "tile.hpp"

#include "../yg/base_texture.hpp"

Tile::Tile()
{}

Tile::Tile(shared_ptr<yg::gl::BaseTexture> const & renderTarget,
     shared_ptr<yg::InfoLayer> const & infoLayer,
     ScreenBase const & tileScreen,
     Tiler::RectInfo const & rectInfo,
     double duration)
  : m_renderTarget(renderTarget),
    m_infoLayer(infoLayer),
    m_tileScreen(tileScreen),
    m_rectInfo(rectInfo),
    m_duration(duration)
{}

Tile::~Tile()
{}
