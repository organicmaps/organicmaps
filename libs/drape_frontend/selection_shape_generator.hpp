#pragma once

#include "drape_frontend/render_node.hpp"

#include "drape/color.hpp"
#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"

#include <vector>

namespace dp
{
class TextureManager;
}  // namespace dp

namespace df
{
class SelectionShapeGenerator
{
public:
  static dp::Color GetSelectionColor();

  static drape_ptr<RenderNode> GenerateSelectionMarker(ref_ptr<dp::GraphicsContext> context,
                                                       ref_ptr<dp::TextureManager> mng);

  static drape_ptr<RenderNode> GenerateTrackSelectionMarker(ref_ptr<dp::GraphicsContext> context,
                                                            ref_ptr<dp::TextureManager> mng);

  /// Builds a selection line render node from an already-read polyline.
  /// Caller is responsible for fetching feature geometry (via MetalineManager / MapDataProvider)
  /// or passing in a pre-built polyline.
  static drape_ptr<RenderNode> GenerateSelectionGeometry(ref_ptr<dp::GraphicsContext> context,
                                                         std::vector<m2::PointD> const & points,
                                                         dp::Color const & color, ref_ptr<dp::TextureManager> mng);
};
}  // namespace df
