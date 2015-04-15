#include "button.hpp"
#include "gui_text.hpp"

#include "drape/batcher.hpp"
#include "drape/shader_def.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "std/bind.hpp"

namespace gui
{

void Button::Draw(Params const & params, ShapeControl & control, dp::RefPointer<dp::TextureManager> texture)
{
  StaticLabel::LabelResult result;
  StaticLabel::CacheStaticText(params.m_label, StaticLabel::DefaultDelim, params.m_anchor,
                               params.m_labelFont, texture, result);

  float textWidth = result.m_boundRect.SizeX();
  float halfWidth = my::clamp(textWidth, params.m_minWidth, params.m_maxWidth) * 0.5f;
  float halfHeight = result.m_boundRect.SizeY() * 0.5f;
  float halfWM = halfWidth + params.m_margin;
  float halfHM = halfHeight + params.m_margin;

  // Cache button
  {
    dp::Color const buttonColor = dp::Color(0x0, 0x0, 0x0, 0x99);
    dp::TextureManager::ColorRegion region;
    texture->GetColorRegion(buttonColor, region);

    dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::Gui);
    state.SetColorTexture(region.GetTexture());

    glsl::vec3 position(0.0f, 0.0f, 0.0f);
    glsl::vec2 texCoord(glsl::ToVec2(region.GetTexRect().Center()));

    gpu::SolidTexturingVertex vertexes[]{
        gpu::SolidTexturingVertex(position, glsl::vec2(-halfWM, halfHM), texCoord),
        gpu::SolidTexturingVertex(position, glsl::vec2(-halfWM, -halfHM), texCoord),
        gpu::SolidTexturingVertex(position, glsl::vec2(halfWM, halfHM), texCoord),
        gpu::SolidTexturingVertex(position, glsl::vec2(halfWM, -halfHM), texCoord)};

    glsl::vec2 normalOffset(0.0f, 0.0f);
    if (params.m_anchor & dp::Left)
      normalOffset.x = halfWidth;
    else if (params.m_anchor & dp::Right)
      normalOffset.x = -halfWidth;

    if (params.m_anchor & dp::Top)
      normalOffset.x = halfHeight;
    else if (params.m_anchor & dp::Bottom)
      normalOffset.x = -halfHeight;

    for (gpu::SolidTexturingVertex & v : vertexes)
      v.m_normal = v.m_normal + normalOffset;

    dp::AttributeProvider provider(1 /* stream count */, ARRAY_SIZE(vertexes));
    provider.InitStream(0 /*stream index*/, gpu::SolidTexturingVertex::GetBindingInfo(),
                        dp::StackVoidRef(vertexes));

    m2::PointF buttonSize(halfWM + halfWM, halfHM + halfHM);
    ASSERT(params.m_bodyHandleCreator, ());
    dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
    dp::SessionGuard guard(batcher, bind(&ShapeControl::AddShape, &control, _1, _2));
    batcher.InsertTriangleStrip(state, dp::MakeStackRefPointer(&provider),
                                params.m_bodyHandleCreator(params.m_anchor, buttonSize));
  }

  // Cache text
  {
    size_t vertexCount = result.m_buffer.size();
    ASSERT(vertexCount % dp::Batcher::VertexPerQuad == 0, ());
    size_t indexCount = dp::Batcher::IndexPerQuad * vertexCount / dp::Batcher::VertexPerQuad;

    dp::AttributeProvider provider(1 /*stream count*/, vertexCount);
    provider.InitStream(0 /*stream index*/, StaticLabel::Vertex::GetBindingInfo(),
                        dp::StackVoidRef(result.m_buffer.data()));

    ASSERT(params.m_labelHandleCreator, ());
    m2::PointF textSize(result.m_boundRect.SizeX(), result.m_boundRect.SizeY());
    dp::MasterPointer<dp::OverlayHandle> handle;
    dp::Batcher batcher(indexCount, vertexCount);
    dp::SessionGuard guard(batcher, bind(&ShapeControl::AddShape, &control, _1, _2));
    batcher.InsertListOfStrip(result.m_state, dp::MakeStackRefPointer(&provider),
                              params.m_labelHandleCreator(params.m_anchor, textSize),
                              dp::Batcher::VertexPerQuad);
  }
}

}
