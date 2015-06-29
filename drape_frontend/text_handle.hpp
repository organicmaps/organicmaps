#pragma once

#include "drape/overlay_handle.hpp"
#include "drape/utils/vertex_decl.hpp"

namespace df
{

class TextHandle : public dp::OverlayHandle
{
public:
  TextHandle(FeatureID const & id, dp::Anchor anchor, double priority)
    : OverlayHandle(id, anchor, priority)
  {}

  TextHandle(FeatureID const & id, dp::Anchor anchor, double priority,
             gpu::TTextDynamicVertexBuffer && normals)
    : OverlayHandle(id, anchor, priority)
    , m_normals(move(normals))
  {}

  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator,
                            ScreenBase const & screen, bool isVisible) const override;

  bool IndexesRequired() const override;

protected:
  gpu::TTextDynamicVertexBuffer m_normals;
};


} // namespace df
