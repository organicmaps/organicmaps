#include "button.hpp"
#include "gui_text.hpp"

#include "drape/batcher.hpp"
#include "drape/shader_def.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "std/bind.hpp"

namespace gui
{

ButtonHandle::ButtonHandle(dp::Anchor anchor, m2::PointF const & size)
  : TBase(anchor, m2::PointF::Zero(), size)
  , m_isInPressedState(false)
{}

void ButtonHandle::OnTapBegin()
{
  m_isInPressedState = true;
}

void ButtonHandle::OnTapEnd()
{
  m_isInPressedState = false;
}

void ButtonHandle::Update(ScreenBase const & screen)
{
  glsl::vec4 color = glsl::ToVec4(m_isInPressedState ? dp::Color(0x0, 0x0, 0x0, 0xCC) :
                                                       dp::Color(0x0, 0x0, 0x0, 0x99));
  m_uniforms.SetFloatValue("u_color", color.r, color.g, color.b, color.a);
  TBase::Update(screen);
}

constexpr int Button::VerticesCount()
{
  return dp::Batcher::VertexPerQuad;
}

void Button::Draw(Params const & params, ShapeControl & control, ref_ptr<dp::TextureManager> texMgr)
{
  StaticLabel::LabelResult result;
  StaticLabel::CacheStaticText(params.m_label, StaticLabel::DefaultDelim, params.m_anchor,
                               params.m_labelFont, texMgr, result);

  float textWidth = result.m_boundRect.SizeX();
  float halfWidth = my::clamp(textWidth, params.m_minWidth, params.m_maxWidth) * 0.5f;
  float halfHeight = result.m_boundRect.SizeY() * 0.5f;
  float halfWM = halfWidth + params.m_margin;
  float halfHM = halfHeight + params.m_margin;

  // Cache button
  {
    dp::GLState state(gpu::BUTTON_PROGRAM, dp::GLState::Gui);

    glsl::vec3 position(0.0f, 0.0f, 0.0f);

    int const verticesCount = VerticesCount();
    ButtonVertex vertexes[verticesCount]
    {
        ButtonVertex(position, glsl::vec2(-halfWM, halfHM)),
        ButtonVertex(position, glsl::vec2(-halfWM, -halfHM)),
        ButtonVertex(position, glsl::vec2(halfWM, halfHM)),
        ButtonVertex(position, glsl::vec2(halfWM, -halfHM))
    };

    glsl::vec2 normalOffset(0.0f, 0.0f);
    if (params.m_anchor & dp::Left)
      normalOffset.x = halfWidth;
    else if (params.m_anchor & dp::Right)
      normalOffset.x = -halfWidth;

    if (params.m_anchor & dp::Top)
      normalOffset.x = halfHeight;
    else if (params.m_anchor & dp::Bottom)
      normalOffset.x = -halfHeight;

    for (ButtonVertex & v : vertexes)
      v.m_normal = v.m_normal + normalOffset;

    dp::AttributeProvider provider(1 /* stream count */, verticesCount);
    provider.InitStream(0 /*stream index*/, ButtonVertex::GetBindingInfo(),
                        make_ref(vertexes));

    m2::PointF buttonSize(halfWM + halfWM, halfHM + halfHM);
    ASSERT(params.m_bodyHandleCreator, ());
    dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
    dp::SessionGuard guard(batcher, bind(&ShapeControl::AddShape, &control, _1, _2));
    batcher.InsertTriangleStrip(state, make_ref(&provider),
                                params.m_bodyHandleCreator(params.m_anchor, buttonSize));
  }

  // Cache text
  {
    size_t vertexCount = result.m_buffer.size();
    ASSERT(vertexCount % dp::Batcher::VertexPerQuad == 0, ());
    size_t indexCount = dp::Batcher::IndexPerQuad * vertexCount / dp::Batcher::VertexPerQuad;

    dp::AttributeProvider provider(1 /*stream count*/, vertexCount);
    provider.InitStream(0 /*stream index*/, StaticLabel::Vertex::GetBindingInfo(),
                        make_ref(result.m_buffer.data()));

    ASSERT(params.m_labelHandleCreator, ());
    m2::PointF textSize(result.m_boundRect.SizeX(), result.m_boundRect.SizeY());

    dp::Batcher batcher(indexCount, vertexCount);
    dp::SessionGuard guard(batcher, bind(&ShapeControl::AddShape, &control, _1, _2));
    batcher.InsertListOfStrip(result.m_state, make_ref(&provider),
                              params.m_labelHandleCreator(params.m_anchor, textSize),
                              dp::Batcher::VertexPerQuad);
  }
}

dp::BindingInfo const & Button::ButtonVertex::GetBindingInfo()
{
  static unique_ptr<dp::BindingInfo> info;

  if (info == nullptr)
  {
    info.reset(new dp::BindingInfo(2));

    dp::BindingDecl & posDecl = info->GetBindingDecl(0);
    posDecl.m_attributeName = "a_position";
    posDecl.m_componentCount = 3;
    posDecl.m_componentType = gl_const::GLFloatType;
    posDecl.m_offset = 0;
    posDecl.m_stride = sizeof(ButtonVertex);

    dp::BindingDecl & normalDecl = info->GetBindingDecl(1);
    normalDecl.m_attributeName = "a_normal";
    normalDecl.m_componentCount = 2;
    normalDecl.m_componentType = gl_const::GLFloatType;
    normalDecl.m_offset = sizeof(glsl::vec3);
    normalDecl.m_stride = posDecl.m_stride;
  }

  return *info.get();
}

}
