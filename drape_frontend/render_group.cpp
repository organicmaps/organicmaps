#include "drape_frontend/render_group.hpp"
#include "drape_frontend/shader_def.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/debug_rect_renderer.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "geometry/screenbase.hpp"

#include "base/stl_add.hpp"

#include <sstream>
#include <utility>

namespace df
{
void BaseRenderGroup::SetRenderParams(ref_ptr<dp::GpuProgram> shader, ref_ptr<dp::GpuProgram> shader3d,
                                      ref_ptr<dp::UniformValuesStorage> generalUniforms)
{
  m_shader = shader;
  m_shader3d = shader3d;
  m_generalUniforms = generalUniforms;
}

void BaseRenderGroup::UpdateAnimation()
{
  m_uniforms.SetFloatValue("u_opacity", 1.0);
}

void BaseRenderGroup::Render(const ScreenBase & screen)
{
  ref_ptr<dp::GpuProgram> shader = screen.isPerspective() ? m_shader3d : m_shader;
  ASSERT(shader != nullptr, ());
  ASSERT(m_generalUniforms != nullptr, ());

  shader->Bind();
  dp::ApplyState(m_state, shader);
  dp::ApplyUniforms(*(m_generalUniforms.get()), shader);
}

RenderGroup::RenderGroup(dp::GLState const & state, df::TileKey const & tileKey)
  : TBase(state, tileKey)
  , m_pendingOnDelete(false)
  , m_canBeDeleted(false)
{}

RenderGroup::~RenderGroup()
{
  m_renderBuckets.clear();
}

void RenderGroup::Update(ScreenBase const & modelView)
{
  ASSERT(m_shader != nullptr, ());
  ASSERT(m_generalUniforms != nullptr, ());
  for (auto & renderBucket : m_renderBuckets)
    renderBucket->Update(modelView);
}

void RenderGroup::CollectOverlay(ref_ptr<dp::OverlayTree> tree)
{
  if (CanBeDeleted())
    return;

  ASSERT(m_shader != nullptr, ());
  ASSERT(m_generalUniforms != nullptr, ());
  for (auto & renderBucket : m_renderBuckets)
    renderBucket->CollectOverlayHandles(tree);
}

bool RenderGroup::HasOverlayHandles() const
{
  for (auto & renderBucket : m_renderBuckets)
  {
    if (renderBucket->HasOverlayHandles())
      return true;
  }
  return false;
}

void RenderGroup::RemoveOverlay(ref_ptr<dp::OverlayTree> tree)
{
  for (auto & renderBucket : m_renderBuckets)
    renderBucket->RemoveOverlayHandles(tree);
}

void RenderGroup::Render(ScreenBase const & screen)
{
  BaseRenderGroup::Render(screen);

  ref_ptr<dp::GpuProgram> shader = screen.isPerspective() ? m_shader3d : m_shader;
  for(auto & renderBucket : m_renderBuckets)
    renderBucket->GetBuffer()->Build(shader);

  // Set tile-based model-view matrix.
  {
    math::Matrix<float, 4, 4> mv = GetTileKey().GetTileBasedModelView(screen);
    m_uniforms.SetMatrix4x4Value("modelView", mv.m_data);
  }

  int const programIndex = m_state.GetProgramIndex();
  int const program3dIndex = m_state.GetProgram3dIndex();

  if (IsOverlay() || GetDepthLayer(m_state) == RenderState::TransitMarkLayer)
  {
    if (programIndex == gpu::COLORED_SYMBOL_PROGRAM ||
        programIndex == gpu::COLORED_SYMBOL_BILLBOARD_PROGRAM)
      GLFunctions::glEnable(gl_const::GLDepthTest);
    else
      GLFunctions::glDisable(gl_const::GLDepthTest);
  }

  auto const & params = df::VisualParams::Instance().GetGlyphVisualParams();
  if (programIndex == gpu::TEXT_OUTLINED_PROGRAM ||
      program3dIndex == gpu::TEXT_OUTLINED_BILLBOARD_PROGRAM)
  {
    m_uniforms.SetFloatValue("u_contrastGamma", params.m_outlineContrast, params.m_outlineGamma);
    m_uniforms.SetFloatValue("u_isOutlinePass", 1.0f);
    dp::ApplyUniforms(m_uniforms, shader);

    for(auto & renderBucket : m_renderBuckets)
      renderBucket->Render(m_state.GetDrawAsLine());

    m_uniforms.SetFloatValue("u_contrastGamma", params.m_contrast, params.m_gamma);
    m_uniforms.SetFloatValue("u_isOutlinePass", 0.0f);
    dp::ApplyUniforms(m_uniforms, shader);
    for(auto & renderBucket : m_renderBuckets)
      renderBucket->Render(m_state.GetDrawAsLine());
  }
  else if (programIndex == gpu::TEXT_PROGRAM ||
           program3dIndex == gpu::TEXT_BILLBOARD_PROGRAM)
  {
    m_uniforms.SetFloatValue("u_contrastGamma", params.m_contrast, params.m_gamma);
    dp::ApplyUniforms(m_uniforms, shader);
    for(auto & renderBucket : m_renderBuckets)
      renderBucket->Render(m_state.GetDrawAsLine());
  }
  else
  {
    dp::ApplyUniforms(m_uniforms, shader);

    for(drape_ptr<dp::RenderBucket> & renderBucket : m_renderBuckets)
      renderBucket->Render(m_state.GetDrawAsLine());
  }

  for(auto const & renderBucket : m_renderBuckets)
    renderBucket->RenderDebug(screen);
}

void RenderGroup::AddBucket(drape_ptr<dp::RenderBucket> && bucket)
{
  m_renderBuckets.push_back(std::move(bucket));
}

bool RenderGroup::IsOverlay() const
{
  auto const depthLayer = GetDepthLayer(m_state);
  return (depthLayer == RenderState::OverlayLayer) ||
         (depthLayer == RenderState::NavigationLayer && HasOverlayHandles());
}

bool RenderGroup::IsUserMark() const
{
  auto const depthLayer = GetDepthLayer(m_state);
  return depthLayer == RenderState::UserLineLayer ||
         depthLayer == RenderState::UserMarkLayer ||
         depthLayer == RenderState::TransitMarkLayer ||
         depthLayer == RenderState::RoutingMarkLayer ||
         depthLayer == RenderState::LocalAdsMarkLayer;
}

bool RenderGroup::UpdateCanBeDeletedStatus(bool canBeDeleted, int currentZoom, ref_ptr<dp::OverlayTree> tree)
{
  if (!IsPendingOnDelete())
    return false;

  for (size_t i = 0; i < m_renderBuckets.size(); )
  {
    bool visibleBucket = !canBeDeleted && (m_renderBuckets[i]->GetMinZoom() <= currentZoom);
    if (!visibleBucket)
    {
      m_renderBuckets[i]->RemoveOverlayHandles(tree);
      swap(m_renderBuckets[i], m_renderBuckets.back());
      m_renderBuckets.pop_back();
    }
    else
    {
      ++i;
    }
  }
  m_canBeDeleted = m_renderBuckets.empty();
  return m_canBeDeleted;
}

bool RenderGroupComparator::operator()(drape_ptr<RenderGroup> const & l, drape_ptr<RenderGroup> const & r)
{
  m_pendingOnDeleteFound |= (l->IsPendingOnDelete() || r->IsPendingOnDelete());
  bool const lCanBeDeleted = l->CanBeDeleted();
  bool const rCanBeDeleted = r->CanBeDeleted();

  if (rCanBeDeleted == lCanBeDeleted)
  {
    auto const & lState = l->GetState();
    auto const & rState = r->GetState();
    auto const lDepth = GetDepthLayer(lState);
    auto const rDepth = GetDepthLayer(rState);
    if (lDepth != rDepth)
      return lDepth < rDepth;

    return lState < rState;
  }
  return rCanBeDeleted;
}

UserMarkRenderGroup::UserMarkRenderGroup(dp::GLState const & state, TileKey const & tileKey)
  : TBase(state, tileKey)
{
  if (state.GetProgramIndex() == gpu::BOOKMARK_ANIM_PROGRAM ||
      state.GetProgram3dIndex() == gpu::BOOKMARK_ANIM_BILLBOARD_PROGRAM)
  {
    m_animation = make_unique<OpacityAnimation>(0.25 /*duration*/, 0.0 /* minValue */, 1.0 /* maxValue*/);
    m_mapping.AddRangePoint(0.6f, 1.3f);
    m_mapping.AddRangePoint(0.85f, 0.8f);
    m_mapping.AddRangePoint(1.0f, 1.0f);
  }
}

void UserMarkRenderGroup::UpdateAnimation()
{
  BaseRenderGroup::UpdateAnimation();
  float interplationT = 1.0f;
  if (m_animation)
  {
    auto const t = static_cast<float>(m_animation->GetOpacity());
    interplationT = m_mapping.GetValue(t);
  }
  m_uniforms.SetFloatValue("u_interpolationT", interplationT);
}

bool UserMarkRenderGroup::IsUserPoint() const
{
  return m_state.GetProgramIndex() != gpu::LINE_PROGRAM;
}

std::string DebugPrint(RenderGroup const & group)
{
  std::ostringstream out;
  out << DebugPrint(group.GetTileKey());
  return out.str();
}
}  // namespace df
