#pragma once

#include "shape.hpp"

#include "../std/function.hpp"
#include "../std/string.hpp"

namespace gui
{
class Button
{
public:
  using TCreatorResult = dp::TransferPointer<dp::OverlayHandle>;
  using THandleCreator = function<TCreatorResult (dp::Anchor, m2::PointF const & size)>;

  struct Params
  {
    string m_label;
    dp::FontDecl m_labelFont;
    dp::Anchor m_anchor;

    float m_minWidth;
    float m_maxWidth;
    float m_margin;
    THandleCreator m_bodyHandleCreator;
    THandleCreator m_labelHandleCreator;
  };

  Button(Params const & params);
  void Draw(ShapeControl & control, dp::RefPointer<dp::TextureManager> texture);

private:
  Params m_params;
};
}
