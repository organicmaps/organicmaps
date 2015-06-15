#include "selection_shape.hpp"
#include "visual_params.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/binding_info.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/shader_def.hpp"
#include "drape/texture_manager.hpp"
#include "drape/uniform_values_storage.hpp"

namespace df
{

namespace
{

struct Vertex
{
  Vertex() = default;
  Vertex(glsl::vec2 const & normal, glsl::vec2 const & texCoord)
    : m_normal(normal)
    , m_texCoord(texCoord)
  {
  }

  glsl::vec2 m_normal;
  glsl::vec2 m_texCoord;
};

dp::BindingInfo GetBindingInfo()
{
  dp::BindingInfo info(2);
  dp::BindingDecl & normal = info.GetBindingDecl(0);
  normal.m_attributeName = "a_normal";
  normal.m_componentCount = 2;
  normal.m_componentType = gl_const::GLFloatType;
  normal.m_offset = 0;
  normal.m_stride = sizeof(Vertex);

  dp::BindingDecl & texCoord = info.GetBindingDecl(1);
  texCoord.m_attributeName = "a_colorTexCoords";
  texCoord.m_componentCount = 2;
  texCoord.m_componentType = gl_const::GLFloatType;
  texCoord.m_offset = sizeof(glsl::vec2);
  texCoord.m_stride = sizeof(Vertex);

  return info;
}

} // namespace

SelectionShape::SelectionShape(ref_ptr<dp::TextureManager> mng)
  : m_position(m2::PointD::Zero())
  , m_animation(false, 0.25)
  , m_selectedObject(OBJECT_EMPTY)
{
  int const TriangleCount = 40;
  int const VertexCount = 3 * TriangleCount;
  float const etalonSector = math::twicePi / static_cast<double>(TriangleCount);

  dp::TextureManager::ColorRegion color;
  mng->GetColorRegion(dp::Color(0x17, 0xBF, 0x8E, 0x6F), color);
  glsl::vec2 colorCoord = glsl::ToVec2(color.GetTexRect().Center());

  buffer_vector<Vertex, TriangleCount> buffer;

  glsl::vec2 startNormal(0.0f, 1.0f);

  for (size_t i = 0; i < TriangleCount + 1; ++i)
  {
    glsl::vec2 normal = glsl::rotate(startNormal, i * etalonSector);
    glsl::vec2 nextNormal = glsl::rotate(startNormal, (i + 1) * etalonSector);

    buffer.emplace_back(startNormal, colorCoord);
    buffer.emplace_back(normal, colorCoord);
    buffer.emplace_back(nextNormal, colorCoord);
  }

  dp::GLState state(gpu::ACCURACY_PROGRAM, dp::GLState::OverlayLayer);
  state.SetColorTexture(color.GetTexture());

  {
    dp::Batcher batcher(TriangleCount * dp::Batcher::IndexPerTriangle, VertexCount);
    dp::SessionGuard guard(batcher, [this](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      drape_ptr<dp::RenderBucket> bucket = move(b);
      ASSERT(bucket->GetOverlayHandlesCount() == 0, ());
      m_renderNode = make_unique_dp<RenderNode>(state, bucket->MoveBuffer());
    });

    dp::AttributeProvider provider(1 /*stream count*/, VertexCount);
    provider.InitStream(0 /*stream index*/, GetBindingInfo(), make_ref(buffer.data()));

    batcher.InsertTriangleList(state, make_ref(&provider), nullptr);
  }

  double r = 15.0f * VisualParams::Instance().GetVisualScale();
  m_mapping.AddRangePoint(0.6, 1.3 * r);
  m_mapping.AddRangePoint(0.85, 0.8 * r);
  m_mapping.AddRangePoint(1.0, r);
}

void SelectionShape::Show(ESelectedObject obj, m2::PointD const & position, bool isAnimate)
{
  m_animation.Hide();
  m_position = position;
  m_selectedObject = obj;
  if (isAnimate)
    m_animation.ShowAnimated();
  else
    m_animation.Show();
}

void SelectionShape::Hide()
{
  m_animation.Hide();
  m_selectedObject = OBJECT_EMPTY;
}

void SelectionShape::Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                            dp::UniformValuesStorage const & commonUniforms)
{
  UNUSED_VALUE(screen);
  ShowHideAnimation::EState state = m_animation.GetState();
  if (state == ShowHideAnimation::STATE_VISIBLE ||
      state == ShowHideAnimation::STATE_SHOW_DIRECTION)
  {
    dp::UniformValuesStorage uniforms = commonUniforms;
    uniforms.SetFloatValue("u_position", m_position.x, m_position.y, 0.0);
    uniforms.SetFloatValue("u_accuracy", m_mapping.GetValue(m_animation.GetT()));
    uniforms.SetFloatValue("u_opacity", 1.0f);
    m_renderNode->Render(mng, uniforms);
  }
}

SelectionShape::ESelectedObject SelectionShape::GetSelectedObject() const
{
  return m_selectedObject;
}

} // namespace df
