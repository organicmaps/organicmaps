#pragma once

#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/metaline_manager.hpp"
#include "drape_frontend/render_node.hpp"

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"

#include "indexer/feature_decl.hpp"

namespace dp
{
class TextureManager;
}  // namespace dp

namespace df
{
class SelectionShapeGenerator
{
public:
  static drape_ptr<RenderNode> GenerateSelectionMarker(ref_ptr<dp::GraphicsContext> context,
                                                       ref_ptr<dp::TextureManager> mng);

  static drape_ptr<RenderNode> GenerateTrackSelectionMarker(ref_ptr<dp::GraphicsContext> context,
                                                            ref_ptr<dp::TextureManager> mng);

  static drape_ptr<RenderNode> GenerateSelectionGeometry(ref_ptr<dp::GraphicsContext> context,
                                                         FeatureID const & feature, ref_ptr<dp::TextureManager> mng,
                                                         ref_ptr<MetalineManager> metalineMng,
                                                         MapDataProvider & mapDataProvider);
};
}  // namespace df
