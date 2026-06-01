#include "drape_frontend/path_text_handle.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/constants.hpp"

#include "base/math.hpp"

namespace df
{

namespace
{
double const kValidPathSplineTurn = 15 * math::pi / 180;
double const kCosTurn = cos(kValidPathSplineTurn);
double constexpr kRoundStep = 23;
int constexpr kMaxStepsCount = 7;
double constexpr kMinSegmentLengthSqr = 1e-12;

size_t GetRoundedSplineReserve(size_t sourcePointCount)
{
  if (sourcePointCount < 2)
    return 2;

  return sourcePointCount + (sourcePointCount - 2) * static_cast<size_t>(kMaxStepsCount);
}

// Append pt to the pixel-space spline, rounding the previous corner with a circular-arc fillet when the
// turn is sharper than kValidPathSplineTurn. The arc is tangent to both segments and turns its tangent
// uniformly, so ceil(turn / kValidPathSplineTurn) equal angular steps is the minimal set of points that
// keeps every sub-turn within the limit the glyph layout (CacheDynamicGeometry) accepts. Very acute
// corners (> kMaxStepsCount * kValidPathSplineTurn) are left sharp so the layout drops text spanning
// them instead of bending it around a large arc unrelated to the road.
void AddPointAndRound(m2::SplineEx & spline, m2::PointD const & pt, double visualScale)
{
  if (spline.GetSize() < 2)
  {
    spline.AddPoint(pt);
    return;
  }

  m2::PointD const p2 = spline.GetPath().back();
  m2::PointD const dir1 = spline.GetDirections().back();
  m2::PointD const seg2 = pt - p2;
  double const len2Sqr = seg2.SquaredLength();
  if (len2Sqr < kMinSegmentLengthSqr)
    return;

  double const len2 = sqrt(len2Sqr);
  m2::PointD const dir2 = seg2 / len2;

  double const dotProduct = m2::DotProduct(dir1, dir2);
  if (dotProduct >= kCosTurn)  // Gentle enough: keep the original vertex.
  {
    spline.AddPoint(pt, dir2, len2);
    return;
  }

  double const turn = acos(std::clamp(dotProduct, -1.0, 1.0));
  if (turn > kMaxStepsCount * kValidPathSplineTurn)  // Too acute: leave it sharp.
  {
    spline.AddPoint(pt, dir2, len2);
    return;
  }

  double const len1 = spline.GetLengths().back();
  double const cut = std::min(kRoundStep * visualScale, 0.5 * std::min(len1, len2));

  m2::PointD const a = p2 - dir1 * cut;  // Tangent point on the incoming segment.
  spline.ReplacePoint(a);                // Replace the sharp corner with the fillet.

  // Generate the arc by rotating the radius vector (a - center) around the arc center in equal steps.
  int const steps = std::max(1, static_cast<int>(ceil(turn / kValidPathSplineTurn)));
  double const sign = m2::CrossProduct(dir1, dir2) >= 0.0 ? 1.0 : -1.0;  // Turn direction (left/right).
  double const radius = cut * tan(0.5 * (math::pi - turn));
  m2::PointD const center = a + m2::PointD(-dir1.y, dir1.x) * (sign * radius);
  double const stepAngle = sign * turn / steps;
  double const cosA = cos(stepAngle);
  double const sinA = sin(stepAngle);
  m2::PointD radial = a - center;
  for (int i = 0; i < steps; ++i)
  {
    radial = m2::PointD(radial.x * cosA - radial.y * sinA, radial.x * sinA + radial.y * cosA);
    spline.AddPoint(center + radial);  // Last iteration lands on p2 + dir2 * cut by construction.
  }

  // Quadratic Bezier alternative, kept for comparison. A Bezier turns its tangent non-uniformly, so it
  // must be oversampled (step ~0.6 of the limit) to stay within it -- roughly twice the points.
  // m2::PointD const b = p2 + dir2 * cut;  // tangent point on the outgoing segment
  // int const bezierSteps = std::max(2, static_cast<int>(ceil(turn / (kValidPathSplineTurn * 0.6))));
  // for (int i = 1; i <= bezierSteps; ++i)
  // {
  //   double const t = static_cast<double>(i) / bezierSteps;
  //   double const u = 1.0 - t;
  //   spline.AddPoint(a * (u * u) + p2 * (2.0 * u * t) + b * (t * t));  // quadratic Bezier a -> p2 -> b
  // }

  spline.AddPoint(pt);
}
}  // namespace

bool IsValidSplineTurn(m2::PointD const & normalizedDir1, m2::PointD const & normalizedDir2)
{
  double const dotProduct = m2::DotProduct(normalizedDir1, normalizedDir2);
  double const kEps = 1e-5;
  return dotProduct > kCosTurn || fabs(dotProduct - kCosTurn) < kEps;
}

void AddPointAndRound(m2::SplineEx & spline, m2::PointD const & pt)
{
  AddPointAndRound(spline, pt, df::VisualParams::Instance().GetVisualScale());
}

PathTextContext::PathTextContext(m2::SharedSpline const & spline, double xOffset)
  : m_globalSpline(spline)
  , m_visualScale(df::VisualParams::Instance().GetVisualScale())
  , m_xOffset(xOffset)
{}

void PathTextContext::SetLayout(drape_ptr<PathTextLayout> && layout, double baseGtoPScale)
{
  m_layout = std::move(layout);
  m_globalOffsets.clear();
  m_globalPivots.clear();
  PathTextLayout::CalculatePositions(m_globalSpline->GetLength(), baseGtoPScale, m_layout->GetPixelLength(),
                                     m_globalOffsets);
  m_globalPivots.reserve(m_globalOffsets.size());
  for (auto const offset : m_globalOffsets)
    m_globalPivots.push_back(m_globalSpline->GetPoint(offset).m_pos);
}

ref_ptr<PathTextLayout> const PathTextContext::GetLayout() const
{
  return make_ref(m_layout);
}

void PathTextContext::BeforeUpdate()
{
  m_updated = false;
}

std::vector<double> const & PathTextContext::GetOffsets() const
{
  return m_globalOffsets;
}

bool PathTextContext::GetGlobalPivot(size_t textIndex, m2::PointD & pivot) const
{
  if (textIndex >= m_globalPivots.size())
    return false;

  auto const & globalPivot = m_globalPivots[textIndex];
  pivot = m2::PointD(globalPivot.x + m_xOffset, globalPivot.y);
  return true;
}

bool PathTextContext::GetPivot(size_t textIndex, ScreenBase const & screen, m2::PointD & pivot,
                               m2::Spline::iterator & centerPointIter)
{
  if (textIndex >= m_pivots.size() || m_pixel3dSplines.empty())
    return false;

  if (!GetGlobalPivot(textIndex, pivot))
    return false;

  auto & pivotInfo = m_pivots[textIndex];
  if (!pivotInfo.m_isCalculated)
  {
    pivotInfo.m_isCalculated = true;
    m2::PointD const pt2d = screen.GtoP(pivot);
    if (!screen.IsReverseProjection3d(pt2d))
    {
      pivotInfo.m_centerPointIter = GetProjectedPoint(screen.PtoP3d(pt2d), m_projectionCursor);
      pivotInfo.m_isValid = pivotInfo.m_centerPointIter.IsAttached();
    }
  }

  if (!pivotInfo.m_isValid)
    return false;

  centerPointIter = pivotInfo.m_centerPointIter;
  return true;
}

void PathTextContext::Update(ScreenBase const & screen)
{
  if (m_updated)
    return;
  m_updated = true;

  m_pixel3dSplines.clear();
  m_pivots.clear();
  m_pivots.resize(m_globalPivots.size());
  m_projectionCursor = {};

  m2::SplineEx pixelSpline(GetRoundedSplineReserve(m_globalSpline->GetSize()));
  for (auto pos : m_globalSpline->GetPath())
  {
    pos.x += m_xOffset;
    pos = screen.GtoP(pos);
    if (screen.IsReverseProjection3d(pos))
    {
      if (pixelSpline.IsValid())
        m_pixel3dSplines.push_back(pixelSpline);
      pixelSpline.Clear();
      continue;
    }
    AddPointAndRound(pixelSpline, screen.PtoP3d(pos), m_visualScale);
  }

  if (pixelSpline.IsValid())
    m_pixel3dSplines.emplace_back(std::move(pixelSpline));

  ASSERT_EQUAL(m_globalPivots.size(), m_globalOffsets.size(), ());
}

m2::Spline::iterator PathTextContext::GetProjectedPoint(m2::PointD const & pt, ProjectionCursor & cursor) const
{
  double minDist = std::numeric_limits<double>::max();
  double resStep = 0.;
  m2::SplineEx const * resSpline = nullptr;
  ProjectionCursor bestCursor;

  auto const updateNearest =
      [&](size_t splineIndex, size_t segmentIndex, double segmentStep, m2::PointD const & nearestPt, double nearestStep)
  {
    double const dist = pt.SquaredLength(nearestPt);
    if (dist >= minDist)
      return false;

    minDist = dist;
    resSpline = &m_pixel3dSplines[splineIndex];
    resStep = nearestStep;
    bestCursor = {splineIndex, segmentIndex, segmentStep};
    return minDist < kMwmPointAccuracy;
  };

  auto const scanSegments = [&](size_t splineIndex, size_t firstSegment, size_t lastSegment, double firstStep)
  {
    auto const & spline = m_pixel3dSplines[splineIndex];
    auto const & path = spline.GetPath();
    auto const & lengths = spline.GetLengths();
    auto const & dirs = spline.GetDirections();

    double step = firstStep;
    for (size_t i = firstSegment; i < lastSegment; ++i)
    {
      double const segLength = lengths[i];
      m2::PointD const & segDir = dirs[i];

      double const t = m2::DotProduct(segDir, pt - path[i]);

      m2::PointD nearestPt;
      double nearestStep;
      if (t <= 0)
      {
        nearestPt = path[i];
        nearestStep = step;
      }
      else if (t >= segLength)
      {
        nearestPt = path[i + 1];
        nearestStep = step + segLength;
      }
      else
      {
        nearestPt = path[i] + segDir * t;
        nearestStep = step + t;
      }

      if (updateNearest(splineIndex, i, step, nearestPt, nearestStep))
        return true;

      step += segLength;
    }

    return false;
  };

  ProjectionCursor const startCursor = cursor;
  for (size_t idx = startCursor.m_splineIndex; idx < m_pixel3dSplines.size(); ++idx)
  {
    auto const & path = m_pixel3dSplines[idx].GetPath();
    ASSERT_GREATER(path.size(), 1, ());

    size_t const segmentCount = path.size() - 1;
    size_t const firstSegment = idx == startCursor.m_splineIndex ? startCursor.m_segmentIndex : 0;
    double const firstStep = idx == startCursor.m_splineIndex ? startCursor.m_segmentStep : 0.0;
    if (scanSegments(idx, firstSegment, segmentCount, firstStep))
      goto end;
  }

  for (size_t idx = 0; idx <= startCursor.m_splineIndex; ++idx)
  {
    auto const & path = m_pixel3dSplines[idx].GetPath();
    ASSERT_GREATER(path.size(), 1, ());

    size_t const segmentCount = path.size() - 1;
    size_t const lastSegment = idx == startCursor.m_splineIndex ? startCursor.m_segmentIndex : segmentCount;
    if (scanSegments(idx, 0, lastSegment, 0.0))
      goto end;
  }

end:
  if (resSpline != nullptr)
    cursor = bestCursor;
  return resSpline ? resSpline->GetPoint(resStep) : m2::Spline::iterator();
}

PathTextHandle::PathTextHandle(dp::OverlayID const & id, std::shared_ptr<PathTextContext> const & context, float depth,
                               uint32_t textIndex, uint64_t priority, ref_ptr<dp::TextureManager> textureManager,
                               int minVisibleScale, bool isBillboard)
  : TextHandle(id, context->GetLayout()->GetGlyphs(), dp::Center, priority, textureManager, minVisibleScale,
               isBillboard)
  , m_context(context)
  , m_textIndex(textIndex)
  , m_depth(depth)
{
  m_buffer.resize(4 * m_context->GetLayout()->GetGlyphCount());
}

bool PathTextHandle::Update(ScreenBase const & screen)
{
  if (!df::TextHandle::Update(screen))
    return false;

  m_hasDynamicGeometry = false;
  if (!m_context->GetGlobalPivot(m_textIndex, m_globalPivot))
    return false;

  if (CanSkipPreciseGeometry(screen))
    return true;

  m_context->Update(screen);

  m2::Spline::iterator centerPointIter;
  if (!m_context->GetPivot(m_textIndex, screen, m_globalPivot, centerPointIter))
    return false;

  if (!m_context->GetLayout()->CacheDynamicGeometry(centerPointIter, m_depth, m_globalPivot, m_buffer))
    return false;

  m_hasDynamicGeometry = true;
  m_dynamicGeometryDirty = true;
  return true;
}

void PathTextHandle::BeforeUpdate()
{
  m_context->BeforeUpdate();
}

m2::RectD PathTextHandle::GetPixelRect(ScreenBase const & screen, bool perspective) const
{
  m2::PointD const pixelPivot(screen.GtoP(m_globalPivot));
  if (!m_hasDynamicGeometry)
    return GetCoarsePixelRect(screen, pixelPivot, perspective);

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
    result.Add(pixelPivot + m2::PointD(glsl::ToPoint(v.m_normal)));

  return result;
}

void PathTextHandle::GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const
{
  m2::PointD const pixelPivot(screen.GtoP(m_globalPivot));
  if (!m_hasDynamicGeometry)
  {
    rects.emplace_back(GetCoarsePixelRect(screen, pixelPivot, perspective));
    return;
  }

  for (size_t quadIndex = 0; quadIndex < m_buffer.size(); quadIndex += 4)
  {
    m2::RectF r;
    r.Add(m2::PointF(pixelPivot) + glsl::ToPoint(m_buffer[quadIndex].m_normal));
    r.Add(m2::PointF(pixelPivot) + glsl::ToPoint(m_buffer[quadIndex + 1].m_normal));
    r.Add(m2::PointF(pixelPivot) + glsl::ToPoint(m_buffer[quadIndex + 2].m_normal));
    r.Add(m2::PointF(pixelPivot) + glsl::ToPoint(m_buffer[quadIndex + 3].m_normal));

    if (perspective)
    {
      if (IsBillboard())
      {
        m2::PointD const pxPivotPerspective = screen.PtoP3d(pixelPivot);

        r.Offset(m2::PointF(-pixelPivot));
        r.Offset(m2::PointF(pxPivotPerspective));
      }
      else
      {
        r = m2::RectF(GetPerspectiveRect(m2::RectD(r), screen));
      }
    }

    bool const needAddRect = perspective ? !screen.IsReverseProjection3d(m2::PointD(r.Center())) : true;
    if (needAddRect)
      rects.emplace_back(std::move(r));
  }
}

void PathTextHandle::GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator) const
{
  // Path text geometry changes during screen movement. Upload it once after a successful Update(),
  // not once per render pass: outlined text renders outline and fill using the same dynamic vertices.
  bool const isVisible = IsVisible();
  SetForceUpdateNormals(isVisible && m_dynamicGeometryDirty);
  TextHandle::GetAttributeMutation(mutator);
  if (isVisible)
    m_dynamicGeometryDirty = false;
}

bool PathTextHandle::Enable3dExtention() const
{
  // Do not extend overlays for path texts.
  return false;
}

bool PathTextHandle::HasLinearFeatureShape() const
{
  return true;
}

m2::RectD PathTextHandle::GetCoarsePixelRect(ScreenBase const & screen, m2::PointD const & pixelPivot,
                                             bool perspective) const
{
  m2::PointD const center =
      perspective && !screen.IsReverseProjection3d(pixelPivot) ? screen.PtoP3d(pixelPivot) : pixelPivot;

  auto const layout = m_context->GetLayout();
  double const radius = 0.5 * layout->GetPixelLength() + layout->GetPixelHeight();
  return m2::RectD(center, radius, radius);
}

bool PathTextHandle::CanSkipPreciseGeometry(ScreenBase const & screen) const
{
  m2::PointD const pixelPivot = screen.GtoP(m_globalPivot);
  if (screen.IsReverseProjection3d(pixelPivot))
    return true;

  // Calc _the same_ rect as in detail::OverlayTraits::SetModelView.
  // This handle will be skipped in OverlayTree rendering anyway.
  m2::RectD screenRect = screen.PixelRectIn3d();
  double const extension = m_context->GetVisualScale() * dp::kScreenPixelRectExtension;
  screenRect.Inflate(extension, extension);
  return !screenRect.IsIntersect(GetCoarsePixelRect(screen, pixelPivot, screen.isPerspective()));
}

}  // namespace df
