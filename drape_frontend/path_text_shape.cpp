#include "drape_frontend/path_text_shape.hpp"
#include "drape_frontend/intrusive_vector.hpp"
#include "drape_frontend/shader_def.hpp"
#include "drape_frontend/text_handle.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glstate.hpp"
#include "drape/overlay_handle.hpp"

#include "base/math.hpp"
#include "base/logging.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"
#include "base/matrix.hpp"

#include "geometry/transformations.hpp"

using m2::Spline;

namespace
{
class PathTextHandle : public df::TextHandle
{
public:
  PathTextHandle(dp::OverlayID const & id, m2::SharedSpline const & spl,
                 df::SharedTextLayout const & layout,
                 float mercatorOffset, float depth, uint32_t textIndex,
                 uint64_t priority, int fixedHeight,
                 ref_ptr<dp::TextureManager> textureManager,
                 bool isBillboard)
    : TextHandle(id, layout->GetText(), dp::Center, priority, fixedHeight,
                 textureManager, isBillboard)
    , m_spline(spl)
    , m_layout(layout)
    , m_textIndex(textIndex)
    , m_globalOffset(mercatorOffset)
    , m_depth(depth)
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

    auto const & globalPoints = m_spline->GetPath();
    m2::Spline pixelSpline(m_spline->GetSize());
    m2::Spline::iterator centerPointIter;

    if (screen.isPerspective())
    {
      // In perspective mode we draw the first label only.
      if (m_textIndex != 0)
        return false;

      float pixelOffset = 0.0f;
      bool foundOffset = false;
      for (auto pos : globalPoints)
      {
        pos = screen.GtoP(pos);
        if (!screen.PixelRect().IsPointInside(pos))
        {
          if ((foundOffset = CalculatePerspectiveOffsets(pixelSpline, m_textIndex, pixelOffset)))
            break;

          pixelSpline = m2::Spline(m_spline->GetSize());
          continue;
        }
        pixelSpline.AddPoint(screen.PtoP3d(pos));
      }

      // We aren't able to place the only label anywhere.
      if (!foundOffset && !CalculatePerspectiveOffsets(pixelSpline, m_textIndex, pixelOffset))
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

  void GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const override
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
        {
          r = m2::RectF(GetPerspectiveRect(m2::RectD(r), screen));
        }
      }

      bool const needAddRect = perspective ? !screen.IsReverseProjection3d(r.Center()) : true;
      if (needAddRect)
        rects.emplace_back(move(r));
    }
  }

  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator) const override
  {
    // for visible text paths we always update normals
    SetForceUpdateNormals(IsVisible());
    TextHandle::GetAttributeMutation(mutator);
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

  bool HasLinearFeatureShape() const override
  {
    return true;
  }

private:
  bool CalculatePerspectiveOffsets(m2::Spline const & pixelSpline, uint32_t textIndex,
                                   float & pixelOffset) const
  {
    if (pixelSpline.GetSize() < 2)
      return false;

    float offset = 0.0f;
    if (!df::PathTextLayout::CalculatePerspectivePosition(static_cast<float>(pixelSpline.GetLength()),
                                                          m_layout->GetPixelLength(), textIndex, offset))
    {
      return false;
    }

    pixelOffset = offset;
    return true;
  }

private:
  m2::SharedSpline m_spline;
  df::SharedTextLayout m_layout;
  uint32_t const m_textIndex;
  m2::PointD m_globalPivot;
  float const m_globalOffset;
  float const m_depth;
};
}  // namespace

namespace df
{
PathTextShape::PathTextShape(m2::SharedSpline const & spline,
                             PathTextViewParams const & params,
                             TileKey const & tileKey, uint32_t baseTextIndex)
  : m_spline(spline)
  , m_params(params)
  , m_tileCoords(tileKey.GetTileCoords())
  , m_baseTextIndex(baseTextIndex)
{}

bool PathTextShape::CalculateLayout(ref_ptr<dp::TextureManager> textures)
{
  std::string text = m_params.m_mainText;
  if (!m_params.m_auxText.empty())
    text += "   " + m_params.m_auxText;

  m_layout = SharedTextLayout(new PathTextLayout(m_params.m_tileCenter,
                                                 strings::MakeUniString(text),
                                                 m_params.m_textFont.m_size,
                                                 m_params.m_textFont.m_isSdf,
                                                 textures));
  uint32_t const glyphCount = m_layout->GetGlyphCount();
  if (glyphCount == 0)
    return false;

  PathTextLayout::CalculatePositions(static_cast<float>(m_spline->GetLength()),
                                     m_params.m_baseGtoPScale, m_layout->GetPixelLength(),
                                     m_offsets);
  return !m_offsets.empty();
}

uint64_t PathTextShape::GetOverlayPriority(uint32_t textIndex, size_t textLength) const
{
  // Overlay priority for path text shapes considers length of the text and index of text.
  // Greater text length has more priority, because smaller texts have more chances to be shown along the road.
  // [6 bytes - standard overlay priority][1 byte - length][1 byte - path text index].

  // Special displacement mode.
  if (m_params.m_specialDisplacement == SpecialDisplacement::SpecialMode)
    return dp::CalculateSpecialModePriority(m_params.m_specialPriority);

  static uint64_t constexpr kMask = ~static_cast<uint64_t>(0xFFFF);
  uint64_t priority = dp::CalculateOverlayPriority(m_params.m_minVisibleScale, m_params.m_rank,
                                                   m_params.m_depth);
  priority &= kMask;
  priority |= (static_cast<uint8_t>(textLength) << 8);
  priority |= static_cast<uint8_t>(textIndex);

  return priority;
}

void PathTextShape::DrawPathTextPlain(ref_ptr<dp::TextureManager> textures,
                                      ref_ptr<dp::Batcher> batcher) const
{
  ASSERT(!m_layout.IsNull(), ());
  ASSERT(!m_offsets.empty(), ());

  dp::TextureManager::ColorRegion color;
  textures->GetColorRegion(m_params.m_textFont.m_color, color);

  dp::GLState state(m_layout->GetFixedHeight() > 0 ? gpu::TEXT_FIXED_PROGRAM : gpu::TEXT_PROGRAM,
                    dp::GLState::OverlayLayer);
  state.SetProgram3dIndex(m_layout->GetFixedHeight() > 0 ? gpu::TEXT_FIXED_BILLBOARD_PROGRAM :
                                                           gpu::TEXT_BILLBOARD_PROGRAM);
  state.SetColorTexture(color.GetTexture());
  state.SetMaskTexture(m_layout->GetMaskTexture());

  gpu::TTextStaticVertexBuffer staticBuffer;
  gpu::TTextDynamicVertexBuffer dynBuffer;

  for (uint32_t textIndex = 0; textIndex < static_cast<uint32_t>(m_offsets.size()); ++textIndex)
  {
    float const offset = m_offsets[textIndex];
    staticBuffer.clear();
    dynBuffer.clear();

    m_layout->CacheStaticGeometry(color, staticBuffer);
    dynBuffer.resize(staticBuffer.size());

    dp::AttributeProvider provider(2, static_cast<uint32_t>(staticBuffer.size()));
    provider.InitStream(0, gpu::TextStaticVertex::GetBindingInfo(),
                        make_ref(staticBuffer.data()));
    provider.InitStream(1, gpu::TextDynamicVertex::GetBindingInfo(),
                        make_ref(dynBuffer.data()));
    batcher->InsertListOfStrip(state, make_ref(&provider),
                               CreateOverlayHandle(m_layout, textIndex, offset, textures), 4);
  }
}

void PathTextShape::DrawPathTextOutlined(ref_ptr<dp::TextureManager> textures,
                                         ref_ptr<dp::Batcher> batcher) const
{
  ASSERT(!m_layout.IsNull(), ());
  ASSERT(!m_offsets.empty(), ());

  dp::TextureManager::ColorRegion color;
  dp::TextureManager::ColorRegion outline;
  textures->GetColorRegion(m_params.m_textFont.m_color, color);
  textures->GetColorRegion(m_params.m_textFont.m_outlineColor, outline);

  dp::GLState state(gpu::TEXT_OUTLINED_PROGRAM, dp::GLState::OverlayLayer);
  state.SetProgram3dIndex(gpu::TEXT_OUTLINED_BILLBOARD_PROGRAM);
  state.SetColorTexture(color.GetTexture());
  state.SetMaskTexture(m_layout->GetMaskTexture());

  gpu::TTextOutlinedStaticVertexBuffer staticBuffer;
  gpu::TTextDynamicVertexBuffer dynBuffer;
  for (uint32_t textIndex = 0; textIndex < static_cast<uint32_t>(m_offsets.size()); ++textIndex)
  {
    float const offset = m_offsets[textIndex];
    staticBuffer.clear();
    dynBuffer.clear();

    m_layout->CacheStaticGeometry(color, outline, staticBuffer);
    dynBuffer.resize(staticBuffer.size());

    dp::AttributeProvider provider(2, static_cast<uint32_t>(staticBuffer.size()));
    provider.InitStream(0, gpu::TextOutlinedStaticVertex::GetBindingInfo(),
                        make_ref(staticBuffer.data()));
    provider.InitStream(1, gpu::TextDynamicVertex::GetBindingInfo(),
                        make_ref(dynBuffer.data()));
    batcher->InsertListOfStrip(state, make_ref(&provider),
                               CreateOverlayHandle(m_layout, textIndex, offset, textures), 4);
  }
}

drape_ptr<dp::OverlayHandle> PathTextShape::CreateOverlayHandle(SharedTextLayout const & layoutPtr,
                                                                uint32_t textIndex, float offset,
                                                                ref_ptr<dp::TextureManager> textures) const
{
  dp::OverlayID const overlayId = dp::OverlayID(m_params.m_featureID, m_tileCoords,
                                                m_baseTextIndex + textIndex);
  auto const priority = GetOverlayPriority(textIndex, layoutPtr->GetText().size());
  return make_unique_dp<PathTextHandle>(overlayId, m_spline, layoutPtr, offset, m_params.m_depth,
                                        textIndex, priority, layoutPtr->GetFixedHeight(),
                                        textures, true /* isBillboard */);
}

void PathTextShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
{
  if (m_layout.IsNull() || m_offsets.empty())
    return;

  if (m_params.m_textFont.m_outlineColor == dp::Color::Transparent())
    DrawPathTextPlain(textures, batcher);
  else
    DrawPathTextOutlined(textures, batcher);
}
}  // namespace df
