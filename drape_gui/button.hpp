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

    float m_minWidth = 0.0f;
    float m_maxWidth = 0.0f;
    float m_margin = 0.0f;
    THandleCreator m_bodyHandleCreator;
    THandleCreator m_labelHandleCreator;
  };

  static void Draw(Params const & params, ShapeControl & control, dp::RefPointer<dp::TextureManager> texture);
};
}
