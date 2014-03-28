#include "poi_symbol_shape.hpp"

#include "../drape/texture_set_holder.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"

#include "../drape/shader_def.hpp"

namespace df
{
  PoiSymbolShape::PoiSymbolShape(const m2::PointD & mercatorPt, PoiSymbolViewParams const & params)
    : m_pt(mercatorPt)
    , m_params(params)
  {
  }

  void PoiSymbolShape::Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const
  {
    TextureSetHolder::TextureRegion region;
    textures->GetSymbolRegion(m_params.m_symbolName, region);

    GLState state(gpu::TEXTURING_PROGRAM, 0);
    state.SetTextureSet(region.m_textureSet);

    state.SetBlending(Blending(true));

    m2::PointF halfSize(region.m_pixelSize.x / 2.0, region.m_pixelSize.y / 2.0);
    m2::RectF const & texRect = region.m_stRect;
    float depth = m_params.m_depth;
    float texture = (float)region.m_textureOffset;

    float stream[]     =
    //   x       y       z      normal.x    normal.y        s              t          textureIndex
    { m_pt.x, m_pt.y, depth, -halfSize.x,  halfSize.y, texRect.minX(), texRect.maxY(), texture,
      m_pt.x, m_pt.y, depth, -halfSize.x, -halfSize.y, texRect.minX(), texRect.minY(), texture,
      m_pt.x, m_pt.y, depth,  halfSize.x,  halfSize.y, texRect.maxX(), texRect.maxY(), texture,
      m_pt.x, m_pt.y, depth,  halfSize.x, -halfSize.y, texRect.maxX(), texRect.minY(), texture};

    AttributeProvider provider(1, 4);
    BindingInfo info(3);

    BindingDecl & posDecl = info.GetBindingDecl(0);
    posDecl.m_attributeName = "a_position";
    posDecl.m_componentCount = 3;
    posDecl.m_componentType = GLConst::GLFloatType;
    posDecl.m_offset = 0;
    posDecl.m_stride = 8 * sizeof(float);

    BindingDecl & normalDecl = info.GetBindingDecl(1);
    normalDecl.m_attributeName = "a_normal";
    normalDecl.m_componentCount = 2;
    normalDecl.m_componentType = GLConst::GLFloatType;
    normalDecl.m_offset = 3 * sizeof(float);
    normalDecl.m_stride = 8 * sizeof(float);

    BindingDecl & texDecl = info.GetBindingDecl(2);
    texDecl.m_attributeName = "a_texCoords";
    texDecl.m_componentCount = 2;
    texDecl.m_componentType = GLConst::GLFloatType;
    texDecl.m_offset = (3 + 2) * sizeof(float);
    texDecl.m_stride = 8 * sizeof(float);

    provider.InitStream(0, info, MakeStackRefPointer<void>(stream));
    batcher->InsertTriangleStrip(state, MakeStackRefPointer(&provider));
  }
}
