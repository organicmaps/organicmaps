#pragma once

#include "drape/color.hpp"
#include "drape/overlay_tree.hpp"

namespace dp
{
class GraphicsContext;

class DebugRenderer
{
public:
  virtual ~DebugRenderer() = default;
  virtual bool IsEnabled() const = 0;
  virtual void DrawRect(ref_ptr<GraphicsContext> context, ScreenBase const & screen, m2::RectF const & rect,
                        Color const & color) = 0;
  virtual void DrawArrow(ref_ptr<GraphicsContext> context, ScreenBase const & screen,
                         OverlayTree::DisplacementData const & data) = 0;
};
}  // namespace dp
