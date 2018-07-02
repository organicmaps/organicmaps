#include "drape_frontend/render_group.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/programs.hpp"

#include "drape/debug_rect_renderer.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "geometry/screenbase.hpp"

#include "base/stl_add.hpp"

#include <algorithm>
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

void RenderGroup::SetOverlayVisibility(bool isVisible)
{
  for (auto & renderBucket : m_renderBuckets)
    renderBucket->SetOverlayVisibility(isVisible);
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
    m_uniforms.SetMatrix4x4Value("u_modelView", mv.m_data);
  }

  auto const program = m_state.GetProgram<gpu::Program>();
  auto const program3d = m_state.GetProgram3d<gpu::Program>();

  // TODO: hide GLDepthTest under the state.
  if (IsOverlay())
  {
    if (program == gpu::Program::ColoredSymbol || program3d == gpu::Program::ColoredSymbolBillboard)
      GLFunctions::glEnable(gl_const::GLDepthTest);
    else
      GLFunctions::glDisable(gl_const::GLDepthTest);
  }

  auto const & params = df::VisualParams::Instance().GetGlyphVisualParams();
  if (program == gpu::Program::TextOutlined || program3d == gpu::Program::TextOutlinedBillboard)
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
  else if (program == gpu::Program::Text || program3d == gpu::Program::TextBillboard)
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
         depthLayer == RenderState::LocalAdsMarkLayer ||
         depthLayer == RenderState::SearchMarkLayer;
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
      std::swap(m_renderBuckets[i], m_renderBuckets.back());
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
  auto const program = state.GetProgram<gpu::Program>();
  auto const program3d = state.GetProgram3d<gpu::Program>();

  if (program == gpu::Program::BookmarkAnim || program3d == gpu::Program::BookmarkAnimBillboard)
  {
    m_animation = make_unique<OpacityAnimation>(0.25 /* duration */, 0.0 /* minValue */, 1.0 /* maxValue */);
    m_mapping.AddRangePoint(0.6f, 1.3f);
    m_mapping.AddRangePoint(0.85f, 0.8f);
    m_mapping.AddRangePoint(1.0f, 1.0f);
  }
}

void UserMarkRenderGroup::UpdateAnimation()
{
  BaseRenderGroup::UpdateAnimation();
  float interpolation = 1.0f;
  if (m_animation)
  {
    auto const t = static_cast<float>(m_animation->GetOpacity());
    interpolation = m_mapping.GetValue(t);
  }
  m_uniforms.SetFloatValue("u_interpolation", interpolation);
}

bool UserMarkRenderGroup::IsUserPoint() const
{
  return m_state.GetProgram<gpu::Program>() != gpu::Program::Line;
}

std::string DebugPrint(RenderGroup const & group)
{
  std::ostringstream out;
  out << DebugPrint(group.GetTileKey());
  return out.str();
}
}  // namespace df
