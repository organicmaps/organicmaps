#include "poi_symbol_shape.hpp"

#include "../drape/texture_set_holder.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"

#include "../drape/shader_def.hpp"

namespace df
{

PoiSymbolShape::PoiSymbolShape(m2::PointD const & mercatorPt, PoiSymbolViewParams const & params)
  : m_pt(mercatorPt)
  , m_params(params)
{
}

void PoiSymbolShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  dp::TextureSetHolder::SymbolRegion region;
  textures->GetSymbolRegion(m_params.m_symbolName, region);

  dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::OverlayLayer);
  state.SetTextureSet(region.GetTextureNode().m_textureSet);

  state.SetBlending(dp::Blending(true));

  m2::PointU pixelSize;
  region.GetPixelSize(pixelSize);
  m2::PointF const halfSize(pixelSize.x / 2.0, pixelSize.y / 2.0);
  m2::RectF const & texRect = region.GetTexRect();
  float const depth = m_params.m_depth;
  float const texture = (float)region.GetTextureNode().m_textureOffset;

  float positions[] = {
    m_pt.x, m_pt.y,
    m_pt.x, m_pt.y,
    m_pt.x, m_pt.y,
    m_pt.x, m_pt.y
  };

  float normals[] = {
    -halfSize.x,  halfSize.y,
    -halfSize.x, -halfSize.y,
     halfSize.x,  halfSize.y,
     halfSize.x, -halfSize.y
  };

  float uvs[] = {
    texRect.minX(), texRect.maxY(), texture, depth,
    texRect.minX(), texRect.minY(), texture, depth,
    texRect.maxX(), texRect.maxY(), texture, depth,
    texRect.maxX(), texRect.minY(), texture, depth
  };

  dp::AttributeProvider provider(3, 4);
  {
    dp::BindingInfo position(1, 1);
    dp::BindingDecl & decl = position.GetBindingDecl(0);
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, position, dp::MakeStackRefPointer<void>(positions));
  }
  {
    dp::BindingInfo normal(1);
    dp::BindingDecl & decl = normal.GetBindingDecl(0);
    decl.m_attributeName = "a_normal";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(1, normal, dp::MakeStackRefPointer<void>(normals));
  }
  {
    dp::BindingInfo texcoord(1);
    dp::BindingDecl & decl = texcoord.GetBindingDecl(0);
    decl.m_attributeName = "a_texCoords";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(2, texcoord, dp::MakeStackRefPointer<void>(uvs));
  }

  dp::OverlayHandle * handle = new dp::SquareHandle(m_params.m_id,
                                                    dp::Center,
                                                    m_pt,
                                                    pixelSize,
                                                    depth);

  batcher->InsertTriangleStrip(state, dp::MakeStackRefPointer(&provider), dp::MovePointer(handle));
}

} // namespace df
