#include "drape_frontend/render_group.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/debug_rect_renderer.hpp"
#include "drape/shader_def.hpp"
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
  if (m_pendingOnDelete || GetOpacity() < 1.0)
    return;

  ASSERT(m_shader != nullptr, ());
  ASSERT(m_generalUniforms != nullptr, ());
  for (auto & renderBucket : m_renderBuckets)
    renderBucket->CollectOverlayHandles(tree);
}

void RenderGroup::Render(ScreenBase const & screen)
{
  BaseRenderGroup::Render(screen);

  ref_ptr<dp::GpuProgram> shader = screen.isPerspective() ? m_shader3d : m_shader;
  for(auto & renderBucket : m_renderBuckets)
    renderBucket->GetBuffer()->Build(shader);

  auto const & params = df::VisualParams::Instance().GetGlyphVisualParams();
  int programIndex = m_state.GetProgramIndex();
  int program3dIndex = m_state.GetProgram3dIndex();
  if (programIndex == gpu::TEXT_OUTLINED_PROGRAM ||
      program3dIndex == gpu::TEXT_OUTLINED_BILLBOARD_PROGRAM)
  {
    m_uniforms.SetFloatValue("u_contrastGamma", params.m_outlineContrast, params.m_outlineGamma);
    m_uniforms.SetFloatValue("u_isOutlinePass", 1.0f);
    dp::ApplyUniforms(m_uniforms, shader);

    for(auto & renderBucket : m_renderBuckets)
      renderBucket->Render(screen);

    m_uniforms.SetFloatValue("u_contrastGamma", params.m_contrast, params.m_gamma);
    m_uniforms.SetFloatValue("u_isOutlinePass", 0.0f);
    dp::ApplyUniforms(m_uniforms, shader);
    for(auto & renderBucket : m_renderBuckets)
      renderBucket->Render(screen);
  }
  else if (programIndex == gpu::TEXT_PROGRAM ||
           program3dIndex == gpu::TEXT_BILLBOARD_PROGRAM)
  {
    m_uniforms.SetFloatValue("u_contrastGamma", params.m_contrast, params.m_gamma);
    dp::ApplyUniforms(m_uniforms, shader);
    for(auto & renderBucket : m_renderBuckets)
      renderBucket->Render(screen);
  }
  else
  {
    dp::ApplyUniforms(m_uniforms, shader);

    for(drape_ptr<dp::RenderBucket> & renderBucket : m_renderBuckets)
      renderBucket->Render(screen);
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

void RenderGroup::UpdateAnimation()
{
  double const opactity = GetOpacity();
  m_uniforms.SetFloatValue("u_opacity", opactity);
}

double RenderGroup::GetOpacity() const
{
  if (m_appearAnimation != nullptr)
    return m_appearAnimation->GetOpacity();

  if (m_disappearAnimation != nullptr)
    return m_disappearAnimation->GetOpacity();

  return 1.0;
}

bool RenderGroup::IsAnimating() const
{
  if (m_appearAnimation && !m_appearAnimation->IsFinished())
    return true;

  if (m_disappearAnimation && !m_disappearAnimation->IsFinished())
    return true;

  return false;
}

void RenderGroup::Appear()
{
  // Commented because of perfomance reasons.
  //if (IsOverlay())
  //{
  //  m_appearAnimation = make_unique<OpacityAnimation>(0.25 /* duration */, 0.0 /* delay */,
  //                                                    0.0 /* startOpacity */, 1.0 /* endOpacity */);
  //}
}

void RenderGroup::Disappear()
{
  // Commented because of perfomance reasons.
  //if (IsOverlay())
  //{
  //  m_disappearAnimation = make_unique<OpacityAnimation>(0.1 /* duration */, 0.1 /* delay */,
  //                                                       1.0 /* startOpacity */, 0.0 /* endOpacity */);
  //}
  //else
  {
    // Create separate disappearing animation for area objects to eliminate flickering.
    if (m_state.GetProgramIndex() == gpu::AREA_PROGRAM ||
        m_state.GetProgramIndex() == gpu::AREA_3D_PROGRAM)
      m_disappearAnimation = make_unique<OpacityAnimation>(0.01 /* duration */, 0.25 /* delay */,
                                                           1.0 /* startOpacity */, 1.0 /* endOpacity */);
  }
}

bool RenderGroupComparator::operator()(drape_ptr<RenderGroup> const & l, drape_ptr<RenderGroup> const & r)
{
  dp::GLState const & lState = l->GetState();
  dp::GLState const & rState = r->GetState();

  if (!l->IsPendingOnDelete() && l->IsEmpty())
    l->DeleteLater();

  if (!r->IsPendingOnDelete() && r->IsEmpty())
    r->DeleteLater();

  bool lPendingOnDelete = l->IsPendingOnDelete();
  bool rPendingOnDelete = r->IsPendingOnDelete();

  if (rPendingOnDelete == lPendingOnDelete)
  {
    dp::GLState::DepthLayer lDepth = lState.GetDepthLayer();
    dp::GLState::DepthLayer rDepth = rState.GetDepthLayer();
    if (lDepth != rDepth)
      return lDepth < rDepth;

    if (my::AlmostEqualULPs(l->GetOpacity(), r->GetOpacity()))
      return lState < rState;
    else
      return l->GetOpacity() > r->GetOpacity();
  }

  if (rPendingOnDelete)
    return true;

  return false;
}

UserMarkRenderGroup::UserMarkRenderGroup(dp::GLState const & state,
                                         TileKey const & tileKey,
                                         drape_ptr<dp::RenderBucket> && bucket)
  : TBase(state, tileKey)
  , m_renderBucket(move(bucket))
  , m_animation(new OpacityAnimation(0.25 /*duration*/, 0.0 /* minValue */, 1.0 /* maxValue*/))
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
  ref_ptr<dp::GpuProgram> shader = screen.isPerspective() ? m_shader3d : m_shader;
  dp::ApplyUniforms(m_uniforms, shader);
  if (m_renderBucket != nullptr)
  {
    m_renderBucket->GetBuffer()->Build(shader);
    m_renderBucket->Render(screen);
  }
}

string DebugPrint(RenderGroup const & group)
{
  ostringstream out;
  out << DebugPrint(group.GetTileKey());
  return out.str();
}

} // namespace df
