#include "drape_frontend/path_text_shape.hpp"
#include "drape_frontend/text_handle.hpp"
#include "drape_frontend/text_layout.hpp"
#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/intrusive_vector.hpp"

#include "drape/shader_def.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/glstate.hpp"
#include "drape/batcher.hpp"

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
                 float const mercatorOffset,
                 float const depth)
    : TextHandle(FeatureID(), dp::Center, depth)
    , m_spline(spl)
    , m_layout(layout)
  {
    m_centerPointIter = m_spline.CreateIterator();
    m_centerPointIter.Advance(mercatorOffset);
    m_normals.resize(4 * m_layout->GetGlyphCount());
  }

  bool Update(ScreenBase const & screen) override
  {
    return m_layout->CacheDynamicGeometry(m_centerPointIter, screen, m_normals);
  }

  m2::RectD GetPixelRect(ScreenBase const & screen) const override
  {
    m2::PointD const pixelPivot(screen.GtoP(m_centerPointIter.m_pos));
    m2::RectD result;
    for (gpu::TextDynamicVertex const & v : m_normals)
      result.Add(pixelPivot + glsl::ToPoint(v.m_normal));

    return result;
  }

  void GetPixelShape(ScreenBase const & screen, Rects & rects) const override
  {
    m2::PointD const pixelPivot(screen.GtoP(m_centerPointIter.m_pos));
    for (size_t quadIndex = 0; quadIndex < m_normals.size(); quadIndex += 4)
    {
      m2::RectF r;
      r.Add(pixelPivot + glsl::ToPoint(m_normals[quadIndex].m_normal));
      r.Add(pixelPivot + glsl::ToPoint(m_normals[quadIndex + 1].m_normal));
      r.Add(pixelPivot + glsl::ToPoint(m_normals[quadIndex + 2].m_normal));
      r.Add(pixelPivot + glsl::ToPoint(m_normals[quadIndex + 3].m_normal));
      rects.push_back(r);
    }
  }

  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator,
                            ScreenBase const & screen) const override
  {
    // for visible text paths we always update normals
    SetForceUpdateNormals(IsVisible());
    TextHandle::GetAttributeMutation(mutator, screen);
  }

private:
  m2::SharedSpline m_spline;
  m2::Spline::iterator m_centerPointIter;

  df::SharedTextLayout m_layout;
};

}

namespace df
{

PathTextShape::PathTextShape(m2::SharedSpline const & spline,
                             PathTextViewParams const & params)
  : m_spline(spline)
  , m_params(params)
{
}

void PathTextShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
{
  unique_ptr<PathTextLayout> layout = make_unique<PathTextLayout>(strings::MakeUniString(m_params.m_text),
                                                                  m_params.m_textFont.m_size, textures);

  uint32_t glyphCount = layout->GetGlyphCount();
  if (glyphCount == 0)
    return;

  //we leave a little space on either side of the text that would
  //remove the comparison for equality of spline portions
  float const TextBorder = 4.0f;
  float const textLength = TextBorder + layout->GetPixelLength();
  float const textHalfLength = textLength / 2.0f;
  float const pathGlbLength = m_spline->GetLength();

  // on next readable scale m_scaleGtoP will be twice
  if (textLength > pathGlbLength * 2 * m_params.m_baseGtoPScale)
    return;

  float const pathLength = m_params.m_baseGtoPScale * m_spline->GetLength();

  /// copied from old code
  /// @todo Choose best constant for minimal space.
  float const etalonEmpty = max(200 * df::VisualParams::Instance().GetVisualScale(), (double)textLength);
  float const minPeriodSize = etalonEmpty + textLength;
  float const twoTextAndEmpty = minPeriodSize + textLength;

  buffer_vector<float, 32> offsets;

  float const scalePtoG = 1.0f / m_params.m_baseGtoPScale;

  if (pathLength < twoTextAndEmpty)
  {
    // if we can't place 2 text and empty part on path
    // we place only one text on center of path
    offsets.push_back(pathGlbLength / 2.0f);

  }
  else if (pathLength < twoTextAndEmpty + minPeriodSize)
  {
    // if we can't place 3 text and 2 empty path
    // we place 2 text with empty space beetwen
    // and some offset from path end
    float const endOffset = (pathLength - (2 * textLength + etalonEmpty)) / 2;

    // division on m_scaleGtoP give as global coord frame (Mercator)
    offsets.push_back((endOffset + textHalfLength) * scalePtoG);
    offsets.push_back((pathLength - (textHalfLength + endOffset)) * scalePtoG);
  }
  else
  {
    // here we place 2 text on the ends of path
    // then we place as much as possible text on center path uniformly
    offsets.push_back(textHalfLength * scalePtoG);
    offsets.push_back((pathLength - textHalfLength) * scalePtoG);
    float const emptySpace = pathLength - 2 * textLength;
    uint32_t textCount = static_cast<uint32_t>(ceil(emptySpace / minPeriodSize));
    float const offset = (emptySpace - textCount * textLength) / (textCount + 1);
    for (size_t i = 0; i < textCount; ++i)
      offsets.push_back((textHalfLength + (textLength + offset) * (i + 1)) * scalePtoG);
  }

  dp::TextureManager::ColorRegion color;
  dp::TextureManager::ColorRegion outline;
  textures->GetColorRegion(m_params.m_textFont.m_color, color);
  textures->GetColorRegion(m_params.m_textFont.m_outlineColor, outline);

  dp::GLState state(gpu::TEXT_PROGRAM, dp::GLState::OverlayLayer);
  state.SetColorTexture(color.GetTexture());
  state.SetMaskTexture(layout->GetMaskTexture());

  ASSERT(!offsets.empty(), ());
  gpu::TTextStaticVertexBuffer staticBuffer;
  gpu::TTextDynamicVertexBuffer dynBuffer;
  SharedTextLayout layoutPtr(layout.release());
  for (float offset : offsets)
  {
    staticBuffer.clear();
    dynBuffer.clear();

    Spline::iterator iter = m_spline.CreateIterator();
    iter.Advance(offset);
    layoutPtr->CacheStaticGeometry(glsl::vec3(glsl::ToVec2(iter.m_pos), m_params.m_depth),
                                   color, outline, staticBuffer);

    dynBuffer.resize(staticBuffer.size(), gpu::TextDynamicVertex(glsl::vec2(0.0, 0.0)));

    dp::AttributeProvider provider(2, staticBuffer.size());
    provider.InitStream(0, gpu::TextStaticVertex::GetBindingInfo(), make_ref(staticBuffer.data()));
    provider.InitStream(1, gpu::TextDynamicVertex::GetBindingInfo(), make_ref(dynBuffer.data()));

    drape_ptr<dp::OverlayHandle> handle = make_unique_dp<PathTextHandle>(m_spline, layoutPtr, offset, m_params.m_depth);
    batcher->InsertListOfStrip(state, make_ref(&provider), move(handle), 4);
  }
}

}
