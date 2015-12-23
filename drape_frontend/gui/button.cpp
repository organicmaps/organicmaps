#include "button.hpp"
#include "gui_text.hpp"

#include "drape/batcher.hpp"
#include "drape/shader_def.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "std/bind.hpp"
#include "std/vector.hpp"

namespace gui
{

namespace
{

void ApplyAnchor(dp::Anchor anchor, vector<Button::ButtonVertex> & vertices, float halfWidth, float halfHeight)
{
  glsl::vec2 normalOffset(0.0f, 0.0f);
  if (anchor & dp::Left)
    normalOffset.x = halfWidth;
  else if (anchor & dp::Right)
    normalOffset.x = -halfWidth;

  if (anchor & dp::Top)
    normalOffset.y = halfHeight;
  else if (anchor & dp::Bottom)
    normalOffset.y = -halfHeight;

  for (Button::ButtonVertex & v : vertices)
    v.m_normal = v.m_normal + normalOffset;
}

uint32_t BuildRect(vector<Button::ButtonVertex> & vertices,
                   glsl::vec2 const & v1, glsl::vec2 const & v2,
                   glsl::vec2 const & v3, glsl::vec2 const & v4)

{
  vertices.push_back(Button::ButtonVertex(v1));
  vertices.push_back(Button::ButtonVertex(v2));
  vertices.push_back(Button::ButtonVertex(v3));

  vertices.push_back(Button::ButtonVertex(v3));
  vertices.push_back(Button::ButtonVertex(v2));
  vertices.push_back(Button::ButtonVertex(v4));

  return dp::Batcher::IndexPerQuad;
}

uint32_t BuildCorner(vector<Button::ButtonVertex> & vertices,
                     glsl::vec2 const & pt, double radius,
                     double angleStart, double angleFinish)
{
  int const kTrianglesCount = 8;
  double const sector = (angleFinish - angleStart) / kTrianglesCount;
  m2::PointD startNormal(0.0f, radius);

  for (size_t i = 0; i < kTrianglesCount; ++i)
  {
    m2::PointD normal = m2::Rotate(startNormal, angleStart + i * sector);
    m2::PointD nextNormal = m2::Rotate(startNormal, angleStart + (i + 1) * sector);

    vertices.push_back(Button::ButtonVertex(pt));
    vertices.push_back(Button::ButtonVertex(pt - glsl::ToVec2(normal)));
    vertices.push_back(Button::ButtonVertex(pt - glsl::ToVec2(nextNormal)));
  }

  return kTrianglesCount * dp::Batcher::IndexPerTriangle;
}

}

ButtonHandle::ButtonHandle(dp::Anchor anchor, m2::PointF const & size,
                           dp::Color const & color, dp::Color const & pressedColor)
  : TBase(anchor, m2::PointF::Zero(), size)
  , m_isInPressedState(false)
  , m_color(color)
  , m_pressedColor(pressedColor)
{}

void ButtonHandle::OnTapBegin()
{
  m_isInPressedState = true;
}

void ButtonHandle::OnTapEnd()
{
  m_isInPressedState = false;
}

bool ButtonHandle::Update(ScreenBase const & screen)
{
  glsl::vec4 color = glsl::ToVec4(m_isInPressedState ? m_pressedColor : m_color);
  m_uniforms.SetFloatValue("u_color", color.r, color.g, color.b, color.a);
  return TBase::Update(screen);
}

StaticLabel::LabelResult Button::PreprocessLabel(Params const & params, ref_ptr<dp::TextureManager> texMgr)
{
  StaticLabel::LabelResult result;
  StaticLabel::CacheStaticText(params.m_label, StaticLabel::DefaultDelim, params.m_anchor,
                               params.m_labelFont, texMgr, result);
  return result;
}

void Button::Draw(Params const & params, ShapeControl & control, gui::StaticLabel::LabelResult & label)
{
  float const halfWidth = params.m_width * 0.5f;
  float const halfHeight = label.m_boundRect.SizeY() * 0.5f;
  float const halfWM = halfWidth + params.m_margin;
  float const halfHM = halfHeight + params.m_margin;

  // Cache button
  {
    dp::GLState state(gpu::BUTTON_PROGRAM, dp::GLState::Gui);

    float w = halfWM - params.m_facet;
    float h = halfHM - params.m_facet;

    vector<ButtonVertex> vertexes;
    vertexes.reserve(114);

    uint32_t indicesCount = 0;

    indicesCount += BuildRect(vertexes, glsl::vec2(-w, halfHM), glsl::vec2(-w, -halfHM),
                                        glsl::vec2(w, halfHM), glsl::vec2(w, -halfHM));

    indicesCount += BuildRect(vertexes, glsl::vec2(-halfWM, h), glsl::vec2(-halfWM, -h),
                                        glsl::vec2(-w, h), glsl::vec2(-w, -h));

    indicesCount += BuildRect(vertexes, glsl::vec2(w, h), glsl::vec2(w, -h),
                                        glsl::vec2(halfWM, h), glsl::vec2(halfWM, -h));

    indicesCount += BuildCorner(vertexes, glsl::vec2(-w, h), params.m_facet,
                                math::pi, 1.5 * math::pi);

    indicesCount += BuildCorner(vertexes, glsl::vec2(-w, -h), params.m_facet,
                                1.5 * math::pi, 2 * math::pi);

    indicesCount += BuildCorner(vertexes, glsl::vec2(w, h), params.m_facet,
                                0.5 * math::pi, math::pi);

    indicesCount += BuildCorner(vertexes, glsl::vec2(w, -h), params.m_facet,
                                0.0, 0.5 * math::pi);

    ApplyAnchor(params.m_anchor, vertexes, halfWidth, halfHeight);

    uint32_t const verticesCount = (uint32_t)vertexes.size();
    dp::AttributeProvider provider(1 /* stream count */, verticesCount);
    provider.InitStream(0 /*stream index*/, ButtonVertex::GetBindingInfo(),
                        make_ref(vertexes.data()));

    m2::PointF buttonSize(halfWM + halfWM, halfHM + halfHM);
    ASSERT(params.m_bodyHandleCreator, ());
    dp::Batcher batcher(indicesCount, verticesCount);
    dp::SessionGuard guard(batcher, bind(&ShapeControl::AddShape, &control, _1, _2));
    batcher.InsertTriangleList(state, make_ref(&provider),
                               params.m_bodyHandleCreator(params.m_anchor, buttonSize));
  }

  // Cache text
  {
    size_t vertexCount = label.m_buffer.size();
    ASSERT(vertexCount % dp::Batcher::VertexPerQuad == 0, ());
    size_t indexCount = dp::Batcher::IndexPerQuad * vertexCount / dp::Batcher::VertexPerQuad;

    dp::AttributeProvider provider(1 /*stream count*/, vertexCount);
    provider.InitStream(0 /*stream index*/, StaticLabel::Vertex::GetBindingInfo(),
                        make_ref(label.m_buffer.data()));

    ASSERT(params.m_labelHandleCreator, ());
    m2::PointF textSize(label.m_boundRect.SizeX(), label.m_boundRect.SizeY());

    dp::Batcher batcher(indexCount, vertexCount);
    dp::SessionGuard guard(batcher, bind(&ShapeControl::AddShape, &control, _1, _2));
    batcher.InsertListOfStrip(label.m_state, make_ref(&provider),
                              params.m_labelHandleCreator(params.m_anchor, textSize, label.m_alphabet),
                              dp::Batcher::VertexPerQuad);
  }
}

dp::BindingInfo const & Button::ButtonVertex::GetBindingInfo()
{
  static unique_ptr<dp::BindingInfo> info;

  if (info == nullptr)
  {
    info.reset(new dp::BindingInfo(1));

    dp::BindingDecl & normalDecl = info->GetBindingDecl(0);
    normalDecl.m_attributeName = "a_normal";
    normalDecl.m_componentCount = 2;
    normalDecl.m_componentType = gl_const::GLFloatType;
    normalDecl.m_offset = 0;
    normalDecl.m_stride = sizeof(ButtonVertex);
  }

  return *info.get();
}

}
