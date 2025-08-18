#include "drape_frontend/path_text_handle.hpp"
#include "drape_frontend/visual_params.hpp"

#include "base/math.hpp"

#include <utility>

namespace df
{

namespace
{
double const kValidPathSplineTurn = 15 * math::pi / 180;
double const kCosTurn = cos(kValidPathSplineTurn);
double const kSinTurn = sin(kValidPathSplineTurn);
double const kRoundStep = 23;
int const kMaxStepsCount = 7;

bool RoundCorner(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3, int leftStepsCount,
                 std::vector<m2::PointD> & roundedCorner)
{
  roundedCorner.clear();

  double p1p2Length = (p2 - p1).Length();
  double const p2p3Length = (p3 - p2).Length();

  auto const dir1 = (p2 - p1) / p1p2Length;
  auto const dir2 = (p3 - p2) / p2p3Length;

  if (IsValidSplineTurn(dir1, dir2))
    return false;

  double const vs = df::VisualParams::Instance().GetVisualScale();
  double const kMinCornerDist = 1.0;
  if ((p3 - p1).SquaredLength() < kMinCornerDist * vs)
  {
    roundedCorner.push_back(p1);
    return false;
  }
  m2::PointD const np1 =
      p2 - dir1 * std::min(kRoundStep * vs, p1p2Length - p1p2Length / std::max(leftStepsCount - 1, 2));
  p1p2Length = (p2 - np1).Length();
  double const cosCorner = m2::DotProduct(-dir1, dir2);
  double const sinCorner = fabs(m2::CrossProduct(-dir1, dir2));

  double const p2p3IntersectionLength = (p1p2Length * kSinTurn) / (kSinTurn * cosCorner + kCosTurn * sinCorner);

  if (p2p3IntersectionLength >= p2p3Length)
  {
    roundedCorner.push_back(np1);
    return false;
  }
  else
  {
    m2::PointD const p = p2 + dir2 * p2p3IntersectionLength;
    roundedCorner.push_back(np1);
    roundedCorner.push_back(p);
    return !IsValidSplineTurn((p - np1).Normalize(), dir2);
  }
}

void ReplaceLastCorner(std::vector<m2::PointD> const & roundedCorner, m2::SplineEx & spline)
{
  if (roundedCorner.empty())
    return;
  spline.ReplacePoint(roundedCorner.front());
  for (size_t i = 1, sz = roundedCorner.size(); i < sz; ++i)
    spline.AddPoint(roundedCorner[i]);
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
  if (spline.GetSize() < 2)
  {
    spline.AddPoint(pt);
    return;
  }

  auto const dir1 = spline.GetDirections().back();
  auto const dir2 = (pt - spline.GetPath().back()).Normalize();

  double const dotProduct = m2::DotProduct(dir1, dir2);
  if (dotProduct < kCosTurn)
  {
    int leftStepsCount = static_cast<int>(acos(dotProduct) / kValidPathSplineTurn);
    std::vector<m2::PointD> roundedCorner;
    while (leftStepsCount > 0 && leftStepsCount <= kMaxStepsCount &&
           RoundCorner(spline.GetPath()[spline.GetSize() - 2], spline.GetPath().back(), pt, leftStepsCount--,
                       roundedCorner))
    {
      ReplaceLastCorner(roundedCorner, spline);
    }
    ReplaceLastCorner(roundedCorner, spline);
  }
  spline.AddPoint(pt);
}

PathTextContext::PathTextContext(m2::SharedSpline const & spline) : m_globalSpline(spline) {}

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

bool PathTextContext::GetPivot(size_t textIndex, m2::PointD & pivot, m2::Spline::iterator & centerPointIter) const
{
  if (textIndex >= m_centerGlobalPivots.size())
    return false;

  pivot = m_centerGlobalPivots[textIndex];
  centerPointIter = m_centerPointIters[textIndex];
  return true;
}

void PathTextContext::Update(ScreenBase const & screen)
{
  if (m_updated)
    return;
  m_updated = true;

  m_pixel3dSplines.clear();
  m_centerPointIters.clear();
  m_centerGlobalPivots.clear();

  m2::SplineEx pixelSpline(m_globalSpline->GetSize());
  for (auto pos : m_globalSpline->GetPath())
  {
    pos = screen.GtoP(pos);
    if (screen.IsReverseProjection3d(pos))
    {
      if (pixelSpline.GetSize() > 1)
        m_pixel3dSplines.push_back(std::move(pixelSpline));
      pixelSpline.Clear();
      continue;
    }
    AddPointAndRound(pixelSpline, screen.PtoP3d(pos));
  }

  if (pixelSpline.GetSize() > 1)
    m_pixel3dSplines.emplace_back(std::move(pixelSpline));

  if (m_pixel3dSplines.empty())
    return;

  ASSERT_EQUAL(m_globalPivots.size(), m_globalOffsets.size(), ());
  for (auto const & pivot : m_globalPivots)
  {
    m2::PointD const pt2d = screen.GtoP(pivot);
    if (!screen.IsReverseProjection3d(pt2d))
    {
      auto projectionIter = GetProjectedPoint(screen.PtoP3d(pt2d));
      if (!projectionIter.IsAttached())
        continue;
      m_centerPointIters.push_back(projectionIter);
      m_centerGlobalPivots.push_back(pivot);
    }
  }
}

m2::Spline::iterator PathTextContext::GetProjectedPoint(m2::PointD const & pt) const
{
  double minDist = std::numeric_limits<double>::max();
  double resStep = 0.;
  m2::SplineEx const * resSpline = nullptr;

  for (auto const & spline : m_pixel3dSplines)
  {
    auto const & path = spline.GetPath();
    if (path.size() < 2)
      continue;

    auto const & lengths = spline.GetLengths();
    auto const & dirs = spline.GetDirections();

    double step = 0;
    for (size_t i = 0, sz = path.size() - 1; i < sz; ++i)
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

      double const dist = pt.SquaredLength(nearestPt);
      if (dist < minDist)
      {
        minDist = dist;
        resSpline = &spline;
        resStep = nearestStep;

        if (minDist < kMwmPointAccuracy)
          goto end;
      }

      step += segLength;
    }
  }

end:
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

  m_context->Update(screen);

  m2::Spline::iterator centerPointIter;
  if (!m_context->GetPivot(m_textIndex, m_globalPivot, centerPointIter))
    return false;

  return m_context->GetLayout()->CacheDynamicGeometry(centerPointIter, m_depth, m_globalPivot, m_buffer);
}

void PathTextHandle::BeforeUpdate()
{
  m_context->BeforeUpdate();
}

m2::RectD PathTextHandle::GetPixelRect(ScreenBase const & screen, bool perspective) const
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
    result.Add(pixelPivot + m2::PointD(glsl::ToPoint(v.m_normal)));

  return result;
}

void PathTextHandle::GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const
{
  m2::PointD const pixelPivot(screen.GtoP(m_globalPivot));
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
  // We always update normals for visible text paths.
  SetForceUpdateNormals(IsVisible());
  TextHandle::GetAttributeMutation(mutator);
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

}  // namespace df
