#include "generator/streets/street_geometry.hpp"

#include "generator/boost_helpers.hpp"

#include "indexer/feature_algo.hpp"

#include "geometry/mercator.hpp"

#include "base/exception.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

#include <boost/geometry.hpp>

namespace generator
{
namespace streets
{
Pin StreetGeometry::GetOrChoosePin() const
{
  if (m_pin)
    return *m_pin;

  if (m_highwayGeometry)
    return m_highwayGeometry->ChoosePin();

  if (m_bindingsGeometry)
    return m_bindingsGeometry->GetCentralBinding();

  UNREACHABLE();
}

m2::RectD StreetGeometry::GetBbox() const
{
  if (m_highwayGeometry)
    return m_highwayGeometry->GetBbox();

  if (m_bindingsGeometry)
    return m_bindingsGeometry->GetBbox();

  if (m_pin)
    return {m_pin->m_position, m_pin->m_position};

  UNREACHABLE();
}

void StreetGeometry::SetPin(Pin && pin)
{
  m_pin = std::move(pin);
}

void StreetGeometry::AddHighwayLine(base::GeoObjectId const & osmId, std::vector<m2::PointD> const & line)
{
  if (!m_highwayGeometry)
    m_highwayGeometry = std::make_unique<HighwayGeometry>();

  m_highwayGeometry->AddLine(osmId, line);
}

void StreetGeometry::AddHighwayArea(base::GeoObjectId const osmId, std::vector<m2::PointD> const & border)
{
  if (!m_highwayGeometry)
    m_highwayGeometry = std::make_unique<HighwayGeometry>();

  m_highwayGeometry->AddArea(osmId, border);
}

void StreetGeometry::AddBinding(base::GeoObjectId const & osmId, m2::PointD const & point)
{
  if (!m_bindingsGeometry)
    m_bindingsGeometry = std::make_unique<BindingsGeometry>();

  m_bindingsGeometry->Add(osmId, point);
}

// HighwayGeometry -------------------------------------------------------------------------------------------

Pin HighwayGeometry::ChoosePin() const
{
  if (!m_areaParts.empty())
    return ChooseAreaPin();

  return ChooseMultilinePin();
}

Pin HighwayGeometry::ChooseMultilinePin() const
{
  auto const & lines = m_multiLine.m_lines;
  CHECK(!lines.empty(), ());
  auto longestLine = lines.cbegin();
  auto longestLineLength = longestLine->CalculateLength();
  for (auto l = std::next(longestLine), end = lines.cend(); l != end; ++l)
  {
    auto const length = l->CalculateLength();
    if (longestLineLength < length)
    {
      longestLine = l;
      longestLineLength = length;
    }
  }

  return ChooseLinePin(*longestLine, longestLineLength / 2);
}

Pin HighwayGeometry::ChooseLinePin(Line const & line, double disposeDistance) const
{
  double length = 0.0;

  for (auto const & segment : line.m_segments)
  {
    auto const & points = segment.m_points;
    CHECK_GREATER_OR_EQUAL(points.size(), 2, ());
    for (auto p = points.cbegin(), end = std::prev(points.cend()); p != end; ++p)
    {
      auto const & p1 = *p;
      auto const & p2 = *std::next(p);
      length += MercatorBounds::DistanceOnEarth(p1, p2);
      if (disposeDistance < length)
        return {p1.Mid(p2), segment.m_osmId};
    }
  }

  UNREACHABLE();
}

Pin HighwayGeometry::ChooseAreaPin() const
{
  CHECK(!m_areaParts.empty(), ());
  auto const largestPart = std::max_element(m_areaParts.cbegin(), m_areaParts.cend(),
                                            base::LessBy(&AreaPart::m_area));
  return {largestPart->m_center, largestPart->m_osmId};
}

void HighwayGeometry::AddLine(base::GeoObjectId const & osmId, std::vector<m2::PointD> const & line)
{
  m_multiLine.Add({osmId, line});
  ExtendLimitRect(line);
}

void HighwayGeometry::AddArea(base::GeoObjectId const & osmId, std::vector<m2::PointD> const & border)
{
  m_areaParts.emplace_back(osmId, border);
  ExtendLimitRect(border);
}

m2::RectD const & HighwayGeometry::GetBbox() const
{
  return m_limitRect;
}

void HighwayGeometry::ExtendLimitRect(std::vector<m2::PointD> const & points)
{
  feature::CalcRect(points, m_limitRect);
}

// HighwayGeometry::MultiLine --------------------------------------------------------------------------------

void HighwayGeometry::MultiLine::Add(LineSegment && segment)
{
  for (auto line = m_lines.begin(), end = m_lines.end(); line != end; ++line) 
  {
    if (line->Add(std::move(segment)))
    {
      if (Recombine(std::move(*line)))
        m_lines.erase(line);
      return;
    }
  }

  m_lines.emplace_back(Line{{std::move(segment)}});
}

bool HighwayGeometry::MultiLine::Recombine(Line && line)
{
  for (auto & l : m_lines)
  {
    if (l.Concatenate(std::move(line)))
      return true;
  }

  return false;
}

// HighwayGeometry::Line -------------------------------------------------------------------------------------

bool HighwayGeometry::Line::Concatenate(Line && other)
{
  // Ignore self-addition.
  if (&other == this)
    return false;

  CHECK(!m_segments.empty(), ());
  CHECK(!m_segments.front().m_points.empty() && !m_segments.back().m_points.empty(), ());
  auto const & thisStart = m_segments.front().m_points.front();
  auto const & thisEnd = m_segments.back().m_points.back();

  CHECK(!other.m_segments.empty(), ());
  CHECK(!other.m_segments.front().m_points.empty() && !other.m_segments.back().m_points.empty(), ());
  auto const & otherStart = other.m_segments.front().m_points.front();
  auto const & otherEnd = other.m_segments.back().m_points.back();

  if (AlmostEqualAbs(thisEnd, otherStart, kCoordEqualityEps))
  {
    m_segments.splice(m_segments.end(), std::move(other.m_segments));
    return true;
  }

  if (AlmostEqualAbs(thisStart, otherEnd, kCoordEqualityEps))
  {
    m_segments.splice(m_segments.begin(), std::move(other.m_segments));
    return true;
  }

  if (AlmostEqualAbs(thisStart, otherStart, kCoordEqualityEps))
  {
    other.Reverse();
    m_segments.splice(m_segments.begin(), std::move(other.m_segments));
    return true;
  }

  if (AlmostEqualAbs(thisEnd, otherEnd, kCoordEqualityEps))
  {
    other.Reverse();
    m_segments.splice(m_segments.end(), std::move(other.m_segments));
    return true;
  }

  return false;
}

void HighwayGeometry::Line::Reverse()
{
  for (auto & segment : m_segments)
    std::reverse(segment.m_points.begin(), segment.m_points.end());
  std::reverse(m_segments.begin(), m_segments.end());
}

bool HighwayGeometry::Line::Add(LineSegment && segment)
{
  CHECK(!m_segments.empty(), ());
  auto const & startSegment = m_segments.front();
  auto const & endSegment = m_segments.back();
  CHECK(!startSegment.m_points.empty(), ());
  auto const & lineStart = startSegment.m_points.front();
  CHECK(!endSegment.m_points.empty(), ());
  auto const & lineEnd = endSegment.m_points.back();

  // Ignore self-addition.
  if (startSegment.m_osmId == segment.m_osmId || endSegment.m_osmId == segment.m_osmId)
    return false;

  CHECK(!segment.m_points.empty(), ());
  auto const & segmentStart = segment.m_points.front();
  auto const & segmentEnd = segment.m_points.back();

  if (AlmostEqualAbs(lineEnd, segmentStart, kCoordEqualityEps))
  {
    m_segments.push_back(std::move(segment));
    return true;
  }

  if (AlmostEqualAbs(lineEnd, segmentEnd, kCoordEqualityEps))
  {
    std::reverse(segment.m_points.begin(), segment.m_points.end());
    m_segments.push_back(std::move(segment));
    return true;
  }

  if (AlmostEqualAbs(lineStart, segmentEnd, kCoordEqualityEps))
  {
    m_segments.push_front(std::move(segment));
    return true;
  }

  if (AlmostEqualAbs(lineStart, segmentStart, kCoordEqualityEps))
  {
    std::reverse(segment.m_points.begin(), segment.m_points.end());
    m_segments.push_front(std::move(segment));
    return true;
  }

  return false;
}

double HighwayGeometry::Line::CalculateLength() const noexcept
{
  double length = 0.0;
  for (auto const & segment : m_segments)
    length += segment.CalculateLength();
  return length;
}

// HighwayGeometry::LineSegment ------------------------------------------------------------------------------

HighwayGeometry::LineSegment::LineSegment(base::GeoObjectId const & osmId,
                                          std::vector<m2::PointD> const & points)
  : m_osmId{osmId}, m_points{points}
{
  CHECK_GREATER_OR_EQUAL(m_points.size(), 2, ());
}

double HighwayGeometry::LineSegment::CalculateLength() const noexcept
{
  if (m_points.size() < 2)
    return 0.0;

  double length = 0.0;
  for (auto p1 = m_points.cbegin(), p2 = std::next(p1); p2 != m_points.cend(); p1 = p2, ++p2)
    length += MercatorBounds::DistanceOnEarth(*p1, *p2);

  return length;
}

// HighwayGeometry::AreaPart ---------------------------------------------------------------------------------

HighwayGeometry::AreaPart::AreaPart(base::GeoObjectId const & osmId, std::vector<m2::PointD> const & polygon)
  : m_osmId{osmId}
{
  CHECK_GREATER_OR_EQUAL(polygon.size(), 3, ());

  auto boostPolygon = boost_helpers::BoostPolygon{};
  for (auto const & p : polygon)
    boost::geometry::append(boostPolygon, boost_helpers::BoostPoint{p.x, p.y});
  boost::geometry::correct(boostPolygon);

  boost_helpers::BoostPoint center;
  boost::geometry::centroid(boostPolygon, center);
  m_center = {center.get<0>(), center.get<1>()};

  m_area = boost::geometry::area(boostPolygon);
}

// BindingsGeometry ------------------------------------------------------------------------------------------

Pin BindingsGeometry::GetCentralBinding() const
{
  CHECK(m_centralBinding, ("no bindings"));
  return *m_centralBinding;
}

m2::RectD const & BindingsGeometry::GetBbox() const
{
  CHECK(m_centralBinding, ("no bindings"));
  return m_limitRect;
}

void BindingsGeometry::ExtendLimitRect(m2::PointD const & point)
{
  m_limitRect.Add(point);
}

void BindingsGeometry::Add(base::GeoObjectId const & osmId, m2::PointD const & point)
{
  ExtendLimitRect(point);

  if (!m_centralBinding)
  {
    m_centralBinding = Pin{point, osmId};
    return;
  }

  auto const bboxCenter = GetBbox().Center();
  auto const centralBindingDistance = MercatorBounds::DistanceOnEarth(m_centralBinding->m_position, bboxCenter);
  auto const pointDistance = MercatorBounds::DistanceOnEarth(point, bboxCenter);
  if (pointDistance < centralBindingDistance)
    m_centralBinding = Pin{point, osmId};
}
}  // namespace streets
}  // namespace generator
