#include "drape_frontend/path_symbol_shape.hpp"
#include "drape_frontend/line_shape_helper.hpp"
#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace df
{
namespace
{
struct PathSample
{
  m2::PointD m_position;
  m2::PointD m_direction;
  size_t m_segment;
};

class SplineSampler
{
public:
  explicit SplineSampler(m2::Spline const & spline) : m_spline(spline)
  {
    auto const size = spline.GetSize();
    ASSERT_GREATER(size, 1, ());
    m_distances.reserve(size);
    m_distances.push_back(0.0);
    for (size_t i = 0; i + 1 < size; ++i)
      m_distances.push_back(m_distances.back() + spline.GetTangentAndLength(i).second);
  }

  double GetLength() const { return m_distances.back(); }

  PathSample GetPoint(double distance) const
  {
    distance = std::clamp(distance, 0.0, GetLength());
    auto const it = std::upper_bound(m_distances.begin(), m_distances.end(), distance);
    size_t const index =
        it == m_distances.end() ? m_distances.size() - 2 : static_cast<size_t>(it - m_distances.begin() - 1);
    auto const [direction, length] = m_spline.GetTangentAndLength(index);
    double const segmentDistance = std::min(distance - m_distances[index], length);
    return {m_spline.GetPath()[index] + direction * segmentDistance, direction, index};
  }

private:
  m2::Spline const & m_spline;
  std::vector<double> m_distances;
};

bool IsValidPlacement(SplineSampler const & sampler, double distance, double halfLength, double maxDeviation)
{
  double const fullLength = sampler.GetLength();
  if (distance < halfLength || distance + halfLength > fullLength)
    return false;

  auto const center = sampler.GetPoint(distance);
  m2::PointD const normal(-center.m_direction.y, center.m_direction.x);
  m2::PointD previous;
  int constexpr kSamples = 8;
  double constexpr kMinDirectionDot = 0.866;  // 30 degrees.
  double constexpr kEps = 1e-12;

  for (int i = 0; i <= kSamples; ++i)
  {
    double const sampleDistance = distance - halfLength + 2.0 * halfLength * i / kSamples;
    auto const sample = sampler.GetPoint(sampleDistance);
    if (std::abs(m2::DotProduct(sample.m_position - center.m_position, normal)) > maxDeviation)
      return false;

    if (i > 0)
    {
      m2::PointD const section = sample.m_position - previous;
      double const length = section.Length();
      if (length > kEps && m2::DotProduct(section / length, center.m_direction) < kMinDirectionDot)
        return false;
    }
    previous = sample.m_position;
  }
  return true;
}

std::optional<double> FindValidPlacement(SplineSampler const & sampler, double targetDistance, double symbolLength,
                                         double maxDeviation, double searchRadius)
{
  double const halfLength = 0.5 * symbolLength;
  if (IsValidPlacement(sampler, targetDistance, halfLength, maxDeviation))
    return targetDistance;

  if (searchRadius <= 0.0)
    return std::nullopt;

  double const searchStep = std::max(0.25 * symbolLength, 1e-12);
  for (double delta = searchStep; delta <= searchRadius; delta += searchStep)
  {
    double const after = targetDistance + delta;
    if (IsValidPlacement(sampler, after, halfLength, maxDeviation))
      return after;

    double const before = targetDistance - delta;
    if (IsValidPlacement(sampler, before, halfLength, maxDeviation))
      return before;
  }
  return std::nullopt;
}
}  // namespace

PathSymbolShape::PathSymbolShape(m2::SharedSpline const & spline, PathSymbolViewParams const & params)
  : m_params(params)
  , m_spline(spline)
{}

void PathSymbolShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                           ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::SymbolRegion region;
  textures->GetSymbolRegion(m_params.m_symbolName, region);
  m2::RectF const & rect = region.GetTexRect();
  auto const lt = glsl::ToVec2(rect.LeftTop());
  auto const lb = glsl::ToVec2(rect.LeftBottom());
  auto const rt = glsl::ToVec2(rect.RightTop());
  auto const rb = glsl::ToVec2(rect.RightBottom());

  m2::PointF const halfPxSize = region.GetPixelSize() * (0.5f * m_params.m_symbolScale);

  gpu::VBUnknownSizeT<gpu::SolidTexturingVertex> buffer;
  double const pixelToGlobalScale = 1.0 / m_params.m_baseGtoPScale;

  auto const addSymbol = [&](m2::PointD const & position, m2::PointD const & direction, size_t segment)
  {
    glsl::vec4 const pivot(glsl::ToVec2(ConvertToLocal(position, m_params.m_tileCenter, kShapeCoordScalar)),
                           m_params.m_depth, 0.0f);

    glsl::vec2 const n = halfPxSize.y * glsl::normalize(glsl::vec2(-direction.y, direction.x));
    glsl::vec2 const d = halfPxSize.x * glsl::normalize(glsl::vec2(direction.x, direction.y));
    glsl::vec2 offset = m_params.m_lateralOffsetPixels * glsl::normalize(glsl::vec2(direction.y, -direction.x));
    if (m_params.m_pointOffsetsPixels.size() == m_spline->GetPath().size())
      offset = InterpolateLinePixelOffset(m_spline->GetPath(), m_params.m_pointOffsetsPixels, segment, position);

    if (!m_params.m_exclusionRects.empty())
    {
      m2::PointD const center = position + m2::PointD(offset.x, offset.y) * pixelToGlobalScale;
      m2::RectD const bounds(center, (std::abs(d.x) + std::abs(n.x)) * pixelToGlobalScale,
                             (std::abs(d.y) + std::abs(n.y)) * pixelToGlobalScale);
      if (std::any_of(m_params.m_exclusionRects.begin(), m_params.m_exclusionRects.end(),
                      [&](auto const & rect) { return rect.IsIntersect(bounds); }))
        return;
    }

    buffer.emplace_back(pivot, offset - d - n, lt);
    buffer.emplace_back(pivot, offset - d + n, lb);
    buffer.emplace_back(pivot, offset + d - n, rt);
    buffer.emplace_back(pivot, offset + d + n, rb);
  };

  double const offset = m_params.m_offset * pixelToGlobalScale;
  double const step = m_params.m_step * pixelToGlobalScale;
  ASSERT_GREATER(step, 0.0, ());

  if (m_params.m_maxPathDeviation < 0.0f)
  {
    m2::Spline::iterator splineIter = m_spline.CreateIterator();
    splineIter.Advance(offset);
    while (!splineIter.BeginAgain())
    {
      addSymbol(splineIter.m_pos, splineIter.m_dir, splineIter.GetIndex());
      splineIter.Advance(step);
    }
  }
  else
  {
    double const symbolLength =
        (m_params.m_placementLength > 0.0f ? m_params.m_placementLength : 2.0f * halfPxSize.x) * pixelToGlobalScale;
    double const maxDeviation = m_params.m_maxPathDeviation * pixelToGlobalScale;
    double const searchRadius = m_params.m_searchRadius * pixelToGlobalScale;
    SplineSampler const sampler(*m_spline);
    double const fullLength = sampler.GetLength();
    for (double target = offset; target <= fullLength; target += step)
    {
      auto const placement = FindValidPlacement(sampler, target, symbolLength, maxDeviation, searchRadius);
      if (placement)
      {
        auto const sample = sampler.GetPoint(*placement);
        addSymbol(sample.m_position, sample.m_direction, sample.m_segment);
      }
    }
  }

  if (buffer.empty())
    return;

  auto state = CreateRenderState(gpu::Program::PathSymbol, m_params.m_depthLayer);
  state.SetColorTexture(region.GetTexture());
  state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
  state.SetTextureIndex(region.GetTextureIndex());

  dp::AttributeProvider provider(1, static_cast<uint32_t>(buffer.size()));
  provider.InitStream(0, gpu::SolidTexturingVertex::GetBindingInfo(), make_ref(buffer.data()));
  batcher->InsertListOfStrip(context, state, make_ref(&provider), 4);
}
}  // namespace df
