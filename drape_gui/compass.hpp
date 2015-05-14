#pragma once

#include "shape.hpp"

namespace gui
{

class Compass : public Shape
{
public:
  Compass(gui::Position const & position, dp::TOverlayHandler const & tapHandler)
    : Shape(position)
    , m_tapHandler(tapHandler)
  {}
  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::TextureManager> tex) const override;

private:
  dp::TOverlayHandler m_tapHandler;
};

}
