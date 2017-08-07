#include "drape_frontend/path_text_handle.hpp"

namespace df
{

PathTextContext::PathTextContext(m2::SharedSpline const & spline)
  : m_globalSpline(spline)
{}

void PathTextContext::SetLayout(drape_ptr<PathTextLayout> && layout, double baseGtoPScale)
{
  m_layout = std::move(layout);
  m_globalOffsets.clear();
  m_globalPivots.clear();
  PathTextLayout::CalculatePositions(m_globalSpline->GetLength(), baseGtoPScale,
                                     m_layout->GetPixelLength(), m_globalOffsets);
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

bool PathTextContext::GetPivot(size_t textIndex, m2::PointD & pivot,
                               m2::Spline::iterator & centerPointIter) const
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

  m2::Spline pixelSpline(m_globalSpline->GetSize());
  for (auto pos : m_globalSpline->GetPath())
  {
    pos = screen.GtoP(pos);
    if (screen.IsReverseProjection3d(pos))
    {
      if (pixelSpline.GetSize() > 1)
      {
        m_pixel3dSplines.push_back(pixelSpline);
        pixelSpline.Clear();
      }
      continue;
    }
    pixelSpline.AddPoint(screen.PtoP3d(pos));
  }

  if (pixelSpline.GetSize() > 1)
    m_pixel3dSplines.emplace_back(std::move(pixelSpline));

  if (m_pixel3dSplines.empty())
    return;

  for (size_t i = 0, sz = m_globalOffsets.size(); i < sz; ++i)
  {
    if (screen.isPerspective())
    {
      m2::PointD const pt2d = screen.GtoP(m_globalPivots[i]);
      if (!screen.IsReverseProjection3d(pt2d))
      {
        m_centerPointIters.push_back(GetProjectedPoint(m_pixel3dSplines, screen.PtoP3d(pt2d)));
        m_centerGlobalPivots.push_back(m_globalPivots[i]);
      }
    }
    else
    {
      m_centerPointIters.push_back(m_pixel3dSplines.front().GetPoint(m_globalOffsets[i] / screen.GetScale()));
      m_centerGlobalPivots.push_back(m_globalPivots[i]);
    }
  }
}

m2::Spline::iterator PathTextContext::GetProjectedPoint(std::vector<m2::Spline> const & splines,
                                                        m2::PointD const & pt) const
{
  m2::Spline::iterator iter;
  double minDist = std::numeric_limits<double>::max();

  for (auto const & spline : splines)
  {
    auto const & path = spline.GetPath();
    if (path.size() < 2)
      continue;

    double step = 0;

    for (size_t i = 0, sz = path.size(); i + 1 < sz; ++i)
    {
      double const segLength = spline.GetLengths()[i];
      m2::PointD const segDir = spline.GetDirections()[i];

      m2::PointD const v = pt - path[i];
      double const t = m2::DotProduct(segDir, v);

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

      double const dist = pt.SquareLength(nearestPt);
      if (dist < minDist)
      {
        minDist = dist;
        iter = spline.GetPoint(nearestStep);

        double const kEps = 1e-5;
        if (minDist < kEps)
          return iter;
      }

      step += segLength;
    }
  }

  return iter;
}


PathTextHandle::PathTextHandle(dp::OverlayID const & id,
                               std::shared_ptr<PathTextContext> const & context,
                               float depth, uint32_t textIndex,
                               uint64_t priority, int fixedHeight,
                               ref_ptr<dp::TextureManager> textureManager,
                               bool isBillboard)
  : TextHandle(id, context->GetLayout()->GetText(), dp::Center, priority,
               fixedHeight, textureManager, isBillboard)
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
    result.Add(pixelPivot + glsl::ToPoint(v.m_normal));

  return result;
}

void PathTextHandle::GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const
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

void PathTextHandle::GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator) const
{
  // We always update normals for visible text paths.
  SetForceUpdateNormals(IsVisible());
  TextHandle::GetAttributeMutation(mutator);
}

uint64_t PathTextHandle::GetPriorityMask() const
{
  return dp::kPriorityMaskManual | dp::kPriorityMaskRank;
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
