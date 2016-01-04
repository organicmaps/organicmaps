#include "drape_frontend/path_text_shape.hpp"
#include "drape_frontend/text_handle.hpp"
#include "drape_frontend/text_layout.hpp"
#include "drape_frontend/intrusive_vector.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glstate.hpp"
#include "drape/shader_def.hpp"
#include "drape/overlay_handle.hpp"

#include "base/math.hpp"
#include "base/logging.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"
#include "base/matrix.hpp"

#include "geometry/transformations.hpp"

#include "std/algorithm.hpp"
#include "std/vector.hpp"

using m2::Spline;

namespace
{

class PathTextHandle : public df::TextHandle
{
public:
  PathTextHandle(m2::SharedSpline const & spl,
                 df::SharedTextLayout const & layout,
                 float mercatorOffset, float depth,
                 uint32_t textIndex, uint64_t priority,
                 uint64_t priorityFollowingMode,
                 ref_ptr<dp::TextureManager> textureManager,
                 bool isBillboard)
    : TextHandle(FeatureID(), layout->GetText(), dp::Center, priority, textureManager, isBillboard)
    , m_spline(spl)
    , m_layout(layout)
    , m_textIndex(textIndex)
    , m_globalOffset(mercatorOffset)
    , m_depth(depth)
    , m_priorityFollowingMode(priorityFollowingMode)
  {

    m2::Spline::iterator centerPointIter = m_spline.CreateIterator();
    centerPointIter.Advance(m_globalOffset);
    m_globalPivot = centerPointIter.m_pos;
    m_buffer.resize(4 * m_layout->GetGlyphCount());
  }

  bool Update(ScreenBase const & screen) override
  {
    if (!df::TextHandle::Update(screen))
      return false;
    
    vector<m2::PointD> const & globalPoints = m_spline->GetPath();
    m2::Spline pixelSpline(m_spline->GetSize());
    m2::Spline::iterator centerPointIter;

    if (screen.isPerspective())
    {
      float pixelOffset = 0.0f;
      uint32_t startIndex = 0;
      bool foundOffset = false;
      for (auto pos : globalPoints)
      {
        pos = screen.GtoP(pos);
        if (!screen.PixelRect().IsPointInside(pos))
        {
          if ((foundOffset = CalculateOffsets(pixelSpline, startIndex, pixelOffset)))
            break;

          pixelSpline = m2::Spline(m_spline->GetSize());
          continue;
        }
        pixelSpline.AddPoint(screen.PtoP3d(pos));
      }

      if (!foundOffset && !CalculateOffsets(pixelSpline, startIndex, pixelOffset))
        return false;

      centerPointIter.Attach(pixelSpline);
      centerPointIter.Advance(pixelOffset);
      m_globalPivot = screen.PtoG(screen.P3dtoP(centerPointIter.m_pos));
    }
    else
    {
      for (auto const & pos : globalPoints)
        pixelSpline.AddPoint(screen.GtoP(pos));

      centerPointIter.Attach(pixelSpline);
      centerPointIter.Advance(m_globalOffset / screen.GetScale());
      m_globalPivot = screen.PtoG(centerPointIter.m_pos);
    }

    if (m_buffer.empty())
      m_buffer.resize(4 * m_layout->GetGlyphCount());
    return m_layout->CacheDynamicGeometry(centerPointIter, m_depth, m_globalPivot, m_buffer);
  }

  m2::RectD GetPixelRect(ScreenBase const & screen, bool perspective) const override
  {
    m2::PointD const pixelPivot(screen.GtoP(m_globalPivot));

    if (perspective)
    {
      if (IsBillboard())
      {
        m2::RectD r = GetPixelRect(screen, false);
        m2::PointD pixelPivotPerspective = screen.PtoP3d(pixelPivot);
        r.Offset(-pixelPivot);
        r.Offset(pixelPivotPerspective);

        return r;
      }
      return GetPixelRectPerspective(screen);
    }

    m2::RectD result;
    for (gpu::TextDynamicVertex const & v : m_buffer)
      result.Add(pixelPivot + glsl::ToPoint(v.m_normal));

    return result;
  }

  void GetPixelShape(ScreenBase const & screen, Rects & rects, bool perspective) const override
  {
    m2::PointD const pixelPivot(screen.GtoP(m_globalPivot));
    for (size_t quadIndex = 0; quadIndex < m_buffer.size(); quadIndex += 4)
    {
      m2::RectF r;
      r.Add(pixelPivot + glsl::ToPoint(m_buffer[quadIndex].m_normal));
      r.Add(pixelPivot + glsl::ToPoint(m_buffer[quadIndex + 1].m_normal));
      r.Add(pixelPivot + glsl::ToPoint(m_buffer[quadIndex + 2].m_normal));
      r.Add(pixelPivot + glsl::ToPoint(m_buffer[quadIndex + 3].m_normal));

      if (perspective)
      {
        if (IsBillboard())
        {
          m2::PointD const pxPivotPerspective = screen.PtoP3d(pixelPivot);

          r.Offset(-pixelPivot);
          r.Offset(pxPivotPerspective);
        }
        else
          r = m2::RectF(GetPerspectiveRect(m2::RectD(r), screen));
      }

      m2::RectD const screenRect = perspective ? screen.PixelRectIn3d() : screen.PixelRect();
      if (screenRect.IsIntersect(m2::RectD(r)))
        rects.emplace_back(move(r));
    }
  }

  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator,
                            ScreenBase const & screen) const override
  {
    // for visible text paths we always update normals
    SetForceUpdateNormals(IsVisible());
    TextHandle::GetAttributeMutation(mutator, screen);
  }

  uint64_t GetPriorityMask() const override
  {
    return dp::kPriorityMaskManual | dp::kPriorityMaskRank;
  }

  bool Enable3dExtention() const override
  {
    // Do not extend overlays for path texts.
    return false;
  }

  uint64_t GetPriorityInFollowingMode() const override
  {
    return m_priorityFollowingMode;
  }

private:
  bool CalculateOffsets(const m2::Spline & pixelSpline, uint32_t & startIndex, float & pixelOffset) const
  {
    if (pixelSpline.GetSize() < 2)
      return false;

    vector<float> offsets;
    df::PathTextLayout::CalculatePositions(offsets, pixelSpline.GetLength(), 1.0,
                                           m_layout->GetPixelLength());

    if (startIndex + offsets.size() <= m_textIndex)
    {
      startIndex += offsets.size();
      return false;
    }

    ASSERT_LESS_OR_EQUAL(startIndex, m_textIndex, ());
    pixelOffset = offsets[m_textIndex - startIndex];
    return true;
  }

private:
  m2::SharedSpline m_spline;
  df::SharedTextLayout m_layout;
  uint32_t const m_textIndex;
  m2::PointD m_globalPivot;
  float const m_globalOffset;
  float const m_depth;
  uint64_t const m_priorityFollowingMode;
};

}

namespace df
{

PathTextShape::PathTextShape(m2::SharedSpline const & spline,
                             PathTextViewParams const & params)
  : m_spline(spline)
  , m_params(params)
{}

uint64_t PathTextShape::GetOverlayPriority(bool followingMode) const
{
  // Overlay priority for path text shapes considers length of the text.
  // Greater test length has more priority, because smaller texts have more chances to be shown along the road.
  // [6 bytes - standard overlay priority][1 byte - reserved][1 byte - length].
  static uint64_t constexpr kMask = ~static_cast<uint64_t>(0xFF);
  uint64_t priority = dp::kPriorityMaskAll;
  if (!followingMode)
    priority = dp::CalculateOverlayPriority(m_params.m_minVisibleScale, m_params.m_rank, m_params.m_depth);
  priority &= kMask;
  priority |= (static_cast<uint8_t>(m_params.m_text.size()));

  return priority;
}

void PathTextShape::DrawPathTextPlain(ref_ptr<dp::TextureManager> textures,
                                      ref_ptr<dp::Batcher> batcher,
                                      unique_ptr<PathTextLayout> && layout,
                                      vector<float> const & offsets) const
{
  dp::TextureManager::ColorRegion color;
  textures->GetColorRegion(m_params.m_textFont.m_color, color);

  dp::GLState state(gpu::TEXT_PROGRAM, dp::GLState::OverlayLayer);
  state.SetProgram3dIndex(gpu::TEXT_BILLBOARD_PROGRAM);
  state.SetColorTexture(color.GetTexture());
  state.SetMaskTexture(layout->GetMaskTexture());

  ASSERT(!offsets.empty(), ());
  gpu::TTextStaticVertexBuffer staticBuffer;
  gpu::TTextDynamicVertexBuffer dynBuffer;
  SharedTextLayout layoutPtr(layout.release());
  for (size_t textIndex = 0; textIndex < offsets.size(); ++textIndex)
  {
    float offset = offsets[textIndex];
    staticBuffer.clear();
    dynBuffer.clear();

    layoutPtr->CacheStaticGeometry(color, staticBuffer);
    dynBuffer.resize(staticBuffer.size());

    dp::AttributeProvider provider(2, staticBuffer.size());
    provider.InitStream(0, gpu::TextStaticVertex::GetBindingInfo(), make_ref(staticBuffer.data()));
    provider.InitStream(1, gpu::TextDynamicVertex::GetBindingInfo(), make_ref(dynBuffer.data()));

    drape_ptr<dp::OverlayHandle> handle = make_unique_dp<PathTextHandle>(m_spline, layoutPtr, offset,
                                                                         m_params.m_depth, textIndex,
                                                                         GetOverlayPriority(false /* followingMode */),
                                                                         GetOverlayPriority(true /* followingMode */),
                                                                         textures, true);
    batcher->InsertListOfStrip(state, make_ref(&provider), move(handle), 4);
  }
}

void PathTextShape::DrawPathTextOutlined(ref_ptr<dp::TextureManager> textures,
                                         ref_ptr<dp::Batcher> batcher,
                                         unique_ptr<PathTextLayout> && layout,
                                         vector<float> const & offsets) const
{
  dp::TextureManager::ColorRegion color;
  dp::TextureManager::ColorRegion outline;
  textures->GetColorRegion(m_params.m_textFont.m_color, color);
  textures->GetColorRegion(m_params.m_textFont.m_outlineColor, outline);

  dp::GLState state(gpu::TEXT_OUTLINED_PROGRAM, dp::GLState::OverlayLayer);
  state.SetProgram3dIndex(gpu::TEXT_OUTLINED_BILLBOARD_PROGRAM);
  state.SetColorTexture(color.GetTexture());
  state.SetMaskTexture(layout->GetMaskTexture());

  ASSERT(!offsets.empty(), ());
  gpu::TTextOutlinedStaticVertexBuffer staticBuffer;
  gpu::TTextDynamicVertexBuffer dynBuffer;
  SharedTextLayout layoutPtr(layout.release());
  for (size_t textIndex = 0; textIndex < offsets.size(); ++textIndex)
  {
    float offset = offsets[textIndex];
    staticBuffer.clear();
    dynBuffer.clear();

    layoutPtr->CacheStaticGeometry(color, outline, staticBuffer);
    dynBuffer.resize(staticBuffer.size());

    dp::AttributeProvider provider(2, staticBuffer.size());
    provider.InitStream(0, gpu::TextOutlinedStaticVertex::GetBindingInfo(), make_ref(staticBuffer.data()));
    provider.InitStream(1, gpu::TextDynamicVertex::GetBindingInfo(), make_ref(dynBuffer.data()));

    drape_ptr<dp::OverlayHandle> handle = make_unique_dp<PathTextHandle>(m_spline, layoutPtr, offset,
                                                                         m_params.m_depth, textIndex,
                                                                         GetOverlayPriority(false /* followingMode */),
                                                                         GetOverlayPriority(true /* followingMode */),
                                                                         textures, true);
    batcher->InsertListOfStrip(state, make_ref(&provider), move(handle), 4);
  }
}

void PathTextShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
{
  unique_ptr<PathTextLayout> layout = make_unique<PathTextLayout>(strings::MakeUniString(m_params.m_text),
                                                                  m_params.m_textFont.m_size, textures);

  uint32_t glyphCount = layout->GetGlyphCount();
  if (glyphCount == 0)
    return;

  vector<float> offsets;
  PathTextLayout::CalculatePositions(offsets, m_spline->GetLength(), m_params.m_baseGtoPScale,
                                     layout->GetPixelLength());
  if (offsets.empty())
    return;

  if (m_params.m_textFont.m_outlineColor == dp::Color::Transparent())
    DrawPathTextPlain(textures, batcher, move(layout), offsets);
  else
    DrawPathTextOutlined(textures, batcher, move(layout), offsets);
}

}
