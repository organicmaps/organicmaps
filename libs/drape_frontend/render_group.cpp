#include "drape_frontend/render_group.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/program_manager.hpp"

#include "drape/vertex_array_buffer.hpp"
#include "drape_frontend/debug_rect_renderer.hpp"

#include "geometry/screenbase.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

namespace df
{
void BaseRenderGroup::UpdateAnimation()
{
  m_params.m_opacity = 1.0f;
}

RenderGroup::RenderGroup(dp::RenderState const & state, df::TileKey const & tileKey)
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
  for (auto & renderBucket : m_renderBuckets)
    renderBucket->Update(modelView);
}

void RenderGroup::CollectOverlay(ref_ptr<dp::OverlayTree> tree)
{
  if (CanBeDeleted())
    return;

  for (auto & renderBucket : m_renderBuckets)
    renderBucket->CollectOverlayHandles(tree);
}

bool RenderGroup::HasOverlayHandles() const
{
  for (auto & renderBucket : m_renderBuckets)
    if (renderBucket->HasOverlayHandles())
      return true;
  return false;
}

void RenderGroup::RemoveOverlay(ref_ptr<dp::OverlayTree> tree)
{
  for (auto & renderBucket : m_renderBuckets)
    if (renderBucket->RemoveOverlayHandles(tree))
      break;
}

void RenderGroup::SetOverlayVisibility(bool isVisible)
{
  for (auto & renderBucket : m_renderBuckets)
    renderBucket->SetOverlayVisibility(isVisible);
}

void RenderGroup::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                         ScreenBase const & screen, FrameValues const & frameValues,
                         ref_ptr<DebugRectRenderer> debugRectRenderer)
{
  auto programPtr = mng->GetProgram(screen.isPerspective() ? m_state.GetProgram3d<gpu::Program>()
                                                           : m_state.GetProgram<gpu::Program>());
  ASSERT(programPtr != nullptr, ());
  programPtr->Bind();
  dp::ApplyState(context, programPtr, m_state);

  for (auto & renderBucket : m_renderBuckets)
    renderBucket->GetBuffer()->Build(context, programPtr);

  auto const program = m_state.GetProgram<gpu::Program>();
  auto const program3d = m_state.GetProgram3d<gpu::Program>();

  // Set frame values to group's params.
  frameValues.SetTo(m_params);

  // Set tile-based model-view matrix.
  {
    math::Matrix<float, 4, 4> mv = GetTileKey().GetTileBasedModelView(screen);
    m_params.m_modelView = glsl::make_mat4(mv.m_data);
  }

  auto const & glyphParams = df::VisualParams::Instance().GetGlyphVisualParams();
  if (program == gpu::Program::TextOutlined || program3d == gpu::Program::TextOutlinedBillboard)
  {
    m_params.m_contrastGamma = glsl::vec2(glyphParams.m_outlineContrast, glyphParams.m_outlineGamma);
    m_params.m_isOutlinePass = 1.0f;

    mng->GetParamsSetter()->Apply(context, programPtr, m_params);
    for (auto & renderBucket : m_renderBuckets)
      renderBucket->Render(context, m_state.GetDrawAsLine());

    m_params.m_contrastGamma = glsl::vec2(glyphParams.m_contrast, glyphParams.m_gamma);
    m_params.m_isOutlinePass = 0.0f;
  }
  else if (program == gpu::Program::Text || program3d == gpu::Program::TextBillboard)
  {
    m_params.m_contrastGamma = glsl::vec2(glyphParams.m_contrast, glyphParams.m_gamma);
  }

  mng->GetParamsSetter()->Apply(context, programPtr, m_params);
  for (auto & renderBucket : m_renderBuckets)
    renderBucket->Render(context, m_state.GetDrawAsLine());

  for (auto const & renderBucket : m_renderBuckets)
    renderBucket->RenderDebug(context, screen, debugRectRenderer);
}

void RenderGroup::AddBucket(drape_ptr<dp::RenderBucket> && bucket)
{
  m_renderBuckets.push_back(std::move(bucket));
}

bool RenderGroup::IsUserMark() const
{
  auto const depthLayer = GetDepthLayer(m_state);
  return depthLayer == DepthLayer::UserLineLayer || depthLayer == DepthLayer::UserMarkLayer ||
         depthLayer == DepthLayer::RoutingBottomMarkLayer || depthLayer == DepthLayer::RoutingMarkLayer ||
         depthLayer == DepthLayer::SearchMarkLayer;
}

bool RenderGroup::UpdateCanBeDeletedStatus(bool canBeDeleted, int currentZoom, ref_ptr<dp::OverlayTree> tree)
{
  if (!IsPendingOnDelete())
    return false;

  for (size_t i = 0; i < m_renderBuckets.size();)
  {
    bool const visibleBucket = !canBeDeleted && (m_renderBuckets[i]->GetMinZoom() <= currentZoom);
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

UserMarkRenderGroup::UserMarkRenderGroup(dp::RenderState const & state, TileKey const & tileKey) : TBase(state, tileKey)
{
  auto const program = state.GetProgram<gpu::Program>();
  auto const program3d = state.GetProgram3d<gpu::Program>();

  if (program == gpu::Program::BookmarkAnim || program3d == gpu::Program::BookmarkAnimBillboard)
  {
    m_animation = std::make_unique<OpacityAnimation>(0.25 /* duration */, 0.0 /* minValue */, 1.0 /* maxValue */);
    m_mapping.AddRangePoint(0.6f, 1.3f);
    m_mapping.AddRangePoint(0.85f, 0.8f);
    m_mapping.AddRangePoint(1.0f, 1.0f);
  }
}

void UserMarkRenderGroup::UpdateAnimation()
{
  BaseRenderGroup::UpdateAnimation();
  m_params.m_interpolation = 1.0f;
  if (m_animation)
  {
    auto const t = static_cast<float>(m_animation->GetOpacity());
    m_params.m_interpolation = m_mapping.GetValue(t);
  }
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
