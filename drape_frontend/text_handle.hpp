#pragma once

#include "drape/overlay_handle.hpp"
#include "drape/utils/vertex_decl.hpp"

namespace df
{

class TextHandle : public dp::OverlayHandle
{
public:
  TextHandle(FeatureID const & id, dp::Anchor anchor, double priority);

  TextHandle(FeatureID const & id, dp::Anchor anchor, double priority,
             gpu::TTextDynamicVertexBuffer && normals);

  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator,
                            ScreenBase const & screen) const override;

  bool IndexesRequired() const override;

  void SetForceUpdateNormals(bool forceUpdate) const;

protected:
  gpu::TTextDynamicVertexBuffer m_normals;
  mutable bool m_forceUpdateNormals;

private:
  mutable bool m_isLastVisible;
};


} // namespace df
