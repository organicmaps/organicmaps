#pragma once

#include "shape.hpp"

namespace gui
{
class CountryStatus : public Shape
{
public:
  CountryStatus(Position const & position,
                dp::TOverlayHandler const & downloadMapHandler,
                dp::TOverlayHandler const & downloadMapRoutingHandler,
                dp::TOverlayHandler const & tryAgainHandler)
    : Shape(position)
    , m_downloadMapHandler(downloadMapHandler)
    , m_downloadMapRoutingHandler(downloadMapRoutingHandler)
    , m_tryAgainHandler(tryAgainHandler)
  {}

  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::TextureManager> tex) const override;

private:
  dp::TOverlayHandler m_downloadMapHandler;
  dp::TOverlayHandler m_downloadMapRoutingHandler;
  dp::TOverlayHandler m_tryAgainHandler;
};

}  // namespace gui
