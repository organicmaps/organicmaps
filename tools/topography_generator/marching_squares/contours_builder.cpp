#include "topography_generator/marching_squares/contours_builder.hpp"

#include "geometry/mercator.hpp"

namespace topography_generator
{
ContoursBuilder::ContoursBuilder(size_t levelsCount, std::string const & debugId)
  : m_levelsCount(levelsCount)
  , m_finalizedContours(levelsCount)
  , m_activeContours(levelsCount)
  , m_debugId(debugId)
{}

void ContoursBuilder::AddSegment(size_t levelInd, ms::LatLon const & beginPos, ms::LatLon const & endPos)
{
  if (beginPos.EqualDxDy(endPos, mercator::kPointEqualityEps))
    return;

  CHECK_LESS(levelInd, m_levelsCount, (m_debugId));

  auto contourItBefore = FindContourWithEndPoint(levelInd, beginPos);
  auto contourItAfter = FindContourWithStartPoint(levelInd, endPos);
  auto const connectStart = contourItBefore != m_activeContours[levelInd].end();
  auto const connectEnd = contourItAfter != m_activeContours[levelInd].end();

  if (connectStart && connectEnd && contourItBefore != contourItAfter)
  {
    std::move(contourItAfter->m_countour.begin(), contourItAfter->m_countour.end(),
              std::back_inserter(contourItBefore->m_countour));
    contourItBefore->m_active = true;
    m_activeContours[levelInd].erase(contourItAfter);
  }
  else if (connectStart)
  {
    contourItBefore->m_countour.push_back(endPos);
    contourItBefore->m_active = true;
  }
  else if (connectEnd)
  {
    contourItAfter->m_countour.push_front(beginPos);
    contourItAfter->m_active = true;
  }
  else
  {
    m_activeContours[levelInd].emplace_back(ContourRaw({beginPos, endPos}));
  }
}

void ContoursBuilder::BeginLine()
{
  for (auto & contoursList : m_activeContours)
    for (auto & activeContour : contoursList)
      activeContour.m_active = false;
}

void ContoursBuilder::EndLine(bool finalLine)
{
  for (size_t levelInd = 0; levelInd < m_levelsCount; ++levelInd)
  {
    auto & contours = m_activeContours[levelInd];
    auto it = contours.begin();
    while (it != contours.end())
    {
      if (!it->m_active || finalLine)
      {
        m_finalizedContours[levelInd].push_back(std::move(it->m_countour));
        it = contours.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }
}

ContoursBuilder::ActiveContourIter ContoursBuilder::FindContourWithStartPoint(size_t levelInd, ms::LatLon const & pos)
{
  auto & contours = m_activeContours[levelInd];
  for (auto it = contours.begin(); it != contours.end(); ++it)
    if (it->m_countour.front().EqualDxDy(pos, mercator::kPointEqualityEps))
      return it;
  return contours.end();
}

ContoursBuilder::ActiveContourIter ContoursBuilder::FindContourWithEndPoint(size_t levelInd, ms::LatLon const & pos)
{
  auto & contours = m_activeContours[levelInd];
  for (auto it = contours.begin(); it != contours.end(); ++it)
    if (it->m_countour.back().EqualDxDy(pos, mercator::kPointEqualityEps))
      return it;
  return contours.end();
}
}  // namespace topography_generator
