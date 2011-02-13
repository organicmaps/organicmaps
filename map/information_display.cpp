#include "../base/SRC_FIRST.hpp"

#include "information_display.hpp"
#include "drawer_yg.hpp"

#include "../indexer/mercator.hpp"
#include "../yg/defines.hpp"
#include "../yg/skin.hpp"

#include "../std/fstream.hpp"
#include "../version/version.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"

InformationDisplay::InformationDisplay()
  : m_headingOrientation(-math::pi / 2)
{
  enablePosition(false);
  enableHeading(false);
  enableDebugPoints(false);
  enableRuler(false);
  enableCenter(false);
  enableDebugInfo(false);
  enableMemoryWarning(false);
  enableBenchmarkInfo(false);
  enableGlobalRect(false);

  for (int i = 0; i < sizeof(m_DebugPts) / sizeof(m2::PointD); ++i)
    m_DebugPts[i] = m2::PointD(0, 0);
}

void InformationDisplay::setScreen(ScreenBase const & screen)
{
  m_screen = screen;
}

void InformationDisplay::setBottomShift(double bottomShift)
{
  m_bottomShift = bottomShift;
}

void InformationDisplay::setOrientation(EOrientation orientation)
{
  switch (orientation)
  {
  case EOrientation0:
    m_headingOrientation = -math::pi / 2;
    break;
  case EOrientation90:
    m_headingOrientation = math::pi;
    break;
  case EOrientation180:
    m_headingOrientation = math::pi / 2;
    break;
  case EOrientation270:
    m_headingOrientation = 0;
    break;
  }
}

void InformationDisplay::setDisplayRect(m2::RectI const & rect)
{
  m_displayRect = rect;
}

void InformationDisplay::enablePosition(bool doEnable)
{
  m_isPositionEnabled = doEnable;
}

void InformationDisplay::setPosition(m2::PointD const & mercatorPos, double confidenceRadius)
{
  enablePosition(true);
  m_position = mercatorPos;
  m_confidenceRadius = confidenceRadius;
}

m2::PointD const & InformationDisplay::position() const
{
  return m_position;
}

void InformationDisplay::drawPosition(DrawerYG * pDrawer)
{
  /// Drawing position and heading
  m2::PointD pxPosition = m_screen.GtoP(m_position);
  pDrawer->drawSymbol(pxPosition, "current-position", yg::EPosCenter, yg::maxDepth);

  double pxConfidenceRadius = pxPosition.Length(m_screen.GtoP(m_position + m2::PointD(m_confidenceRadius, 0)));

  pDrawer->screen()->drawArc(pxPosition, 0, math::pi * 2, pxConfidenceRadius, yg::Color(0, 0, 255, 64), yg::maxDepth - 2);
  pDrawer->screen()->fillSector(pxPosition, 0, math::pi * 2, pxConfidenceRadius, yg::Color(0, 0, 255, 32), yg::maxDepth - 3);
}

void InformationDisplay::enableHeading(bool doEnable)
{
  m_isHeadingEnabled = doEnable;
}

void InformationDisplay::setHeading(double trueHeading, double magneticHeading, double accuracy)
{
  enableHeading(true);
  m_trueHeading = trueHeading;
  m_magneticHeading = magneticHeading;
  m_headingAccuracy = accuracy;
}

void InformationDisplay::drawHeading(DrawerYG *pDrawer)
{
  double trueHeadingRad = m_trueHeading / 180 * math::pi;
  double headingAccuracyRad = m_headingAccuracy / 180 * math::pi;

  m2::PointD pxPosition = m_screen.GtoP(m_position);

  double pxConfidenceRadius = pxPosition.Length(m_screen.GtoP(m_position + m2::PointD(m_confidenceRadius, 0)));

  /// true heading
  pDrawer->screen()->drawSector(pxPosition,
                                trueHeadingRad + m_headingOrientation - headingAccuracyRad,
                                trueHeadingRad + m_headingOrientation + headingAccuracyRad,
                                pxConfidenceRadius,
                                yg::Color(255, 0, 0, 64),
                                yg::maxDepth);
  pDrawer->screen()->fillSector(pxPosition,
                                trueHeadingRad + m_headingOrientation - headingAccuracyRad,
                                trueHeadingRad + m_headingOrientation + headingAccuracyRad,
                                pxConfidenceRadius,
                                yg::Color(255, 0, 0, 32),
                                yg::maxDepth - 1);
  /*        /// magnetic heading
      double magneticHeadingRad = m_magneticHeading / 180 * math::pi;
      pDrawer->screen()->drawSector(pxPosition,
                                    magneticHeadingRad + m_headingOrientation - headingAccuracyRad,
                                    magneticHeadingRad + m_headingOrientation + headingAccuracyRad,
                                    pxConfidenceRadius,
                                    yg::Color(0, 255, 0, 64),
                                    yg::maxDepth);
      pDrawer->screen()->fillSector(pxPosition,
                                    magneticHeadingRad + m_headingOrientation - headingAccuracyRad,
                                    magneticHeadingRad + m_headingOrientation + headingAccuracyRad,
                                    pxConfidenceRadius,
                                    yg::Color(0, 255, 0, 32),
                                    yg::maxDepth - 1);
 */
}

void InformationDisplay::enableDebugPoints(bool doEnable)
{
  m_isDebugPointsEnabled = doEnable;
}

void InformationDisplay::setDebugPoint(int pos, m2::PointD const & pt)
{
  m_DebugPts[pos] = pt;
}

void InformationDisplay::drawDebugPoints(DrawerYG * pDrawer)
{
  for (int i = 0; i < sizeof(m_DebugPts) / sizeof(m2::PointD); ++i)
    if (m_DebugPts[i] != m2::PointD(0, 0))
    {
    pDrawer->screen()->drawArc(m_DebugPts[i], 0, math::pi * 2, 30, yg::Color(0, 0, 255, 32), yg::maxDepth);
    pDrawer->screen()->fillSector(m_DebugPts[i], 0, math::pi * 2, 30, yg::Color(0, 0, 255, 32), yg::maxDepth);
  }
}

void InformationDisplay::enableRuler(bool doEnable)
{
  m_isRulerEnabled = doEnable;
}

void InformationDisplay::drawRuler(DrawerYG * pDrawer)
{
  /// Compute Scaler
  /// scaler should be between minPixSize and maxPixSize
  int minPixSize = 80;

  m2::PointD pt0 = m_centerPt;
  m2::PointD pt1 = m_screen.PtoG(m_screen.GtoP(m_centerPt) + m2::PointD(minPixSize, 0));

  double latDiff = fabs(MercatorBounds::YToLat(pt1.x) - MercatorBounds::YToLat(pt0.x));
  double metresDiff = latDiff / MercatorBounds::degreeInMetres;

  /// finding the closest higher metric value
  unsigned curFirstDigit = 2;
  unsigned curVal = 20;
  unsigned maxVal = 1000000;
  bool lessThanMin = false;
  bool isInfinity = false;

  if (metresDiff > maxVal)
  {
    isInfinity = true;
    curVal = maxVal;
  }
  else
  if (metresDiff < curVal)
    lessThanMin = true;
  else
    while (true)
    {
      unsigned nextVal = curFirstDigit == 2 ? (curVal * 5 / 2) : curVal * 2;
      unsigned nextFirstDigit = curFirstDigit == 2 ? (curFirstDigit * 5 / 2) : curFirstDigit * 2;

      if (nextFirstDigit >= 10)
        nextFirstDigit /= 10;

      if ((curVal <= metresDiff) && (nextVal > metresDiff))
      {
        curVal = nextVal;
        curFirstDigit = nextFirstDigit;
        break;
      }

      curVal = nextVal;
      curFirstDigit = nextFirstDigit;
    }

  /// translating meters to pixels
  double scalerWidthLatDiff = (double)curVal * MercatorBounds::degreeInMetres;
  double scalerWidthXDiff = MercatorBounds::LatToY(pt0.x + scalerWidthLatDiff) - MercatorBounds::LatToY(pt0.x);

  double scalerWidthInPx = m_screen.GtoP(pt0).x - m_screen.GtoP(pt0 + m2::PointD(scalerWidthXDiff, 0)).x;
  scalerWidthInPx = (lessThanMin || isInfinity) ? minPixSize : abs(my::rounds(scalerWidthInPx));

  string scalerText;

  if (isInfinity)
    scalerText = ">";
  else
    if (lessThanMin)
      scalerText = "<";

    if (curVal >= 1000)
      scalerText += utils::to_string(curVal / 1000) + " km";
    else
      scalerText += utils::to_string(curVal) + " m";

  m2::PointD scalerOrg = m2::PointD(m_displayRect.minX(), m_displayRect.maxY() - m_bottomShift * m_visualScale) + m2::PointD(10 * m_visualScale, -10 * m_visualScale);

  m2::PointD scalerPts[4];
  scalerPts[0] = scalerOrg + m2::PointD(0, -14 * m_visualScale);
  scalerPts[1] = scalerOrg;
  scalerPts[2] = scalerOrg + m2::PointD(scalerWidthInPx, 0);
  scalerPts[3] = scalerPts[2] + m2::PointD(0, -14 * m_visualScale);

  pDrawer->screen()->drawPath(
      scalerPts, 4,
      pDrawer->screen()->skin()->mapPenInfo(yg::PenInfo(yg::Color(0, 0, 0, 255), 2, 0, 0, 0)),
      yg::maxDepth);

  m2::RectD textRect = pDrawer->screen()->textRect(scalerText.c_str(), 10, true, false);
  pDrawer->screen()->drawText(scalerPts[1] + m2::PointD(7, -7),
                              0,
                              10,
                              yg::Color(0, 0, 0, 0),
                              scalerText.c_str(),
                              true,
                              yg::Color(255, 255, 255, 255),
                              yg::maxDepth,
                              true,
                              false);

/*  m2::PointD minPixPath[4];
  minPixPath[0] = scalerOrg + m2::PointD(0, -14);
  minPixPath[1] = scalerOrg;
  minPixPath[2] = scalerOrg + m2::PointD(minPixSize, 0);
  minPixPath[3] = minPixPath[2] + m2::PointD(0, -14);

  pDrawer->screen()->drawPath(
      minPixPath, 4,
      pDrawer->screen()->skin()->mapPenInfo(yg::PenInfo(yg::Color(255, 0, 0, 255), 4, 0, 0, 0)),
      yg::maxDepth);
 */
}

void InformationDisplay::setVisualScale(double visualScale)
{
  m_visualScale = visualScale;
}

void InformationDisplay::enableCenter(bool doEnable)
{
  m_isCenterEnabled = doEnable;
}

void InformationDisplay::setCenter(m2::PointD const & pt)
{
  m_centerPt = pt;
}

void InformationDisplay::drawCenter(DrawerYG * drawer)
{
  m_yOffset += 20;
  ostringstream out;
  out << "(" << m_centerPt.x << ", " << m_centerPt.y << ") Scale : " << m_currentScale;
  drawer->screen()->drawText(
      m2::PointD(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset),
      0, 10,
      yg::Color(0, 0, 0, 0),
      out.str().c_str(),
      true,
      yg::Color(255, 255, 255, 255),
      yg::maxDepth,
      true,
      false);
}

void InformationDisplay::enableGlobalRect(bool doEnable)
{
  m_isGlobalRectEnabled = doEnable;
}

void InformationDisplay::setGlobalRect(m2::RectD const & r)
{
  m_globalRect = r;
}

void InformationDisplay::drawGlobalRect(DrawerYG *pDrawer)
{
  m_yOffset += 20;
  ostringstream out;
  out << "(" << m_globalRect.minX() << ", " << m_globalRect.minY() << ", " << m_globalRect.maxX() << ", " << m_globalRect.maxY() << ") Scale : " << m_currentScale;
  pDrawer->screen()->drawText(
      m2::PointD(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset),
      0, 10,
      yg::Color(0, 0, 0, 0),
      out.str().c_str(),
      true,
      yg::Color(255, 255, 255, 255),
      yg::maxDepth,
      true,
      false);
}

void InformationDisplay::enableDebugInfo(bool doEnable)
{
  m_isDebugInfoEnabled = doEnable;
}

void InformationDisplay::setDebugInfo(double frameDuration, int currentScale)
{
  m_frameDuration = frameDuration;
  m_currentScale = currentScale;
}

void InformationDisplay::drawDebugInfo(DrawerYG * drawer)
{
  ostringstream out;
  out << "SPF: " << m_frameDuration;
  if (m_frameDuration == 0.0)
    out << " FPS: inf";
  else
    out << " FPS: " << 1.0 / m_frameDuration;

  m_yOffset += 20;

  m2::PointD pos = m2::PointD(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset);

  drawer->screen()->drawText(pos,
                             0, 10,
                             yg::Color(0, 0, 0, 0),
                             out.str().c_str(),
                             true,
                             yg::Color(255, 255, 255, 255),
                             yg::maxDepth,
                             true,
                             false);
}

void InformationDisplay::enableMemoryWarning(bool flag)
{
  m_isMemoryWarningEnabled = flag;
}

void InformationDisplay::memoryWarning()
{
  enableMemoryWarning(true);
  m_lastMemoryWarning = my::Timer();
}

void InformationDisplay::drawMemoryWarning(DrawerYG * drawer)
{
  m_yOffset += 20;
  m2::PointD pos(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset);

  ostringstream out;
  out << "MemoryWarning : " << m_lastMemoryWarning.ElapsedSeconds() << " sec. ago.";

  drawer->screen()->drawText(pos,
                             0, 10,
                             yg::Color(0, 0, 0, 0),
                             out.str().c_str(),
                             true,
                             yg::Color(255, 255, 255, 255),
                             yg::maxDepth,
                             true,
                             false);

  if (m_lastMemoryWarning.ElapsedSeconds() > 5)
    enableMemoryWarning(false);
}

void InformationDisplay::enableBenchmarkInfo(bool doEnable)
{
  m_isBenchmarkInfoEnabled = doEnable;
}

bool InformationDisplay::addBenchmarkInfo(string const & name, m2::RectD const & globalRect, double frameDuration)
{
  if (frameDuration == 0)
    return false;
  if ((m_benchmarkInfo.empty())
    ||(m_benchmarkInfo.back().m_duration != frameDuration))
    {
      BenchmarkInfo info;
      info.m_name = name;
      info.m_duration = frameDuration;
      info.m_rect = globalRect;
      m_benchmarkInfo.push_back(info);

      string deviceID = GetPlatform().DeviceID();
      transform(deviceID.begin(), deviceID.end(), deviceID.begin(), ::tolower);

      ofstream fout(GetPlatform().WritablePathForFile(deviceID + "_benchmark_results.txt").c_str(), ios::app);
      fout << GetPlatform().DeviceID() << " "
           << VERSION_STRING << " "
           << info.m_name << " "
           << info.m_rect.minX() << " "
           << info.m_rect.minY() << " "
           << info.m_rect.maxX() << " "
           << info.m_rect.maxY() << " "
           << info.m_duration << endl;

      return true;
    }
  return false;
}

void InformationDisplay::drawBenchmarkInfo(DrawerYG * pDrawer)
{
  m_yOffset += 20;
  m2::PointD pos(m_displayRect.minX() + 10, m_displayRect.minY() + m_yOffset);
  pDrawer->screen()->drawText(pos,
                              0, 10,
                              yg::Color(0, 0, 0, 0),
                              "benchmark info :",
                              true,
                              yg::Color(255, 255, 255, 255),
                              yg::maxDepth,
                              true,
                              false);

  for (unsigned i = 0; i < m_benchmarkInfo.size(); ++i)
  {
    ostringstream out;
    m2::RectD const & r = m_benchmarkInfo[i].m_rect;
    out << "  " << m_benchmarkInfo[i].m_name
                << ", " << "rect: (" << r.minX()
                << ", " << r.minY()
                << ", " << r.maxX()
                << ", " << r.maxY()
                << "), duration : " << m_benchmarkInfo[i].m_duration;
    m_yOffset += 20;
    pos.y += 20;
    pDrawer->screen()->drawText(pos,
                                0, 10,
                                yg::Color(0, 0, 0, 0),
                                out.str().c_str(),
                                true,
                                yg::Color(255, 255, 255, 255),
                                yg::maxDepth,
                                true,
                                false
                                );
  }

}

void InformationDisplay::doDraw(DrawerYG *drawer)
{
  m_yOffset = 0;
  if (m_isHeadingEnabled)
    drawHeading(drawer);
  if (m_isPositionEnabled)
    drawPosition(drawer);
  if (m_isDebugPointsEnabled)
    drawDebugPoints(drawer);
  if (m_isRulerEnabled)
    drawRuler(drawer);
  if (m_isCenterEnabled)
    drawCenter(drawer);
  if (m_isGlobalRectEnabled)
    drawGlobalRect(drawer);
  if (m_isDebugInfoEnabled)
    drawDebugInfo(drawer);
  if (m_isMemoryWarningEnabled)
    drawMemoryWarning(drawer);
  if (m_isBenchmarkInfoEnabled)
    drawBenchmarkInfo(drawer);
}
