#include "drape_frontend/render_group.hpp"
#include "drape_frontend/shader_def.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/debug_rect_renderer.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "geometry/screenbase.hpp"

#include "base/stl_add.hpp"

#include "std/bind.hpp"

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

bool BaseRenderGroup::IsOverlay() const
{
  return m_state.GetDepthLayer() == dp::GLState::OverlayLayer;
}

RenderGroup::RenderGroup(dp::GLState const & state, df::TileKey const & tileKey)
  : TBase(state, tileKey)
  , m_pendingOnDelete(false)
  , m_canBeDeleted(false)
{
}

RenderGroup::~RenderGroup()
{
  m_renderBuckets.clear();
}

void RenderGroup::Update(ScreenBase const & modelView)
{
  ASSERT(m_shader != nullptr, ());
  ASSERT(m_generalUniforms != nullptr, ());
  for(drape_ptr<dp::RenderBucket> & renderBucket : m_renderBuckets)
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

  if (m_state.GetDepthLayer() == dp::GLState::OverlayLayer)
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

#ifdef RENDER_DEBUG_RECTS
  for(auto const & renderBucket : m_renderBuckets)
    renderBucket->RenderDebug(screen);
#endif
}

void RenderGroup::AddBucket(drape_ptr<dp::RenderBucket> && bucket)
{
  m_renderBuckets.push_back(move(bucket));
}

bool RenderGroup::IsLess(RenderGroup const & other) const
{
  return m_state < other.m_state;
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
    dp::GLState const & lState = l->GetState();
    dp::GLState const & rState = r->GetState();
    dp::GLState::DepthLayer lDepth = lState.GetDepthLayer();
    dp::GLState::DepthLayer rDepth = rState.GetDepthLayer();
    if (lDepth != rDepth)
      return lDepth < rDepth;

    return lState < rState;
  }

  if (rCanBeDeleted)
    return true;

  return false;
}

UserMarkRenderGroup::UserMarkRenderGroup(size_t layerId, dp::GLState const & state, TileKey const & tileKey,
                                         drape_ptr<dp::RenderBucket> && bucket)
  : TBase(state, tileKey)
  , m_renderBucket(move(bucket))
  , m_animation(new OpacityAnimation(0.25 /*duration*/, 0.0 /* minValue */, 1.0 /* maxValue*/))
  , m_layerId(layerId)
{
  m_mapping.AddRangePoint(0.6, 1.3);
  m_mapping.AddRangePoint(0.85, 0.8);
  m_mapping.AddRangePoint(1.0, 1.0);
}

UserMarkRenderGroup::~UserMarkRenderGroup()
{
}

void UserMarkRenderGroup::UpdateAnimation()
{
  BaseRenderGroup::UpdateAnimation();
  float t = 1.0;
  if (m_animation)
    t = m_animation->GetOpacity();

  m_uniforms.SetFloatValue("u_interpolationT", m_mapping.GetValue(t));
}

void UserMarkRenderGroup::Render(ScreenBase const & screen)
{
  BaseRenderGroup::Render(screen);

  // Set tile-based model-view matrix.
  {
    math::Matrix<float, 4, 4> mv = GetTileKey().GetTileBasedModelView(screen);
    m_uniforms.SetMatrix4x4Value("modelView", mv.m_data);
  }

  ref_ptr<dp::GpuProgram> shader = screen.isPerspective() ? m_shader3d : m_shader;
  dp::ApplyUniforms(m_uniforms, shader);
  if (m_renderBucket != nullptr)
  {
    m_renderBucket->GetBuffer()->Build(shader);
    m_renderBucket->Render(m_state.GetDrawAsLine());
  }
}

size_t UserMarkRenderGroup::GetLayerId() const
{
  return m_layerId;
}

bool UserMarkRenderGroup::CanBeClipped() const
{
  return m_state.GetProgramIndex() != gpu::LINE_PROGRAM;
}

string DebugPrint(RenderGroup const & group)
{
  ostringstream out;
  out << DebugPrint(group.GetTileKey());
  return out.str();
}

} // namespace df
