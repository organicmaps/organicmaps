#include "qt/ruler.hpp"

#include "qt/qt_common/translations.hpp"

#include "geometry/mercator.hpp"

#include <QtCore/QLocale>

#include <string>

namespace qt
{
bool Ruler::IsActive()
{
  return m_isActive;
}

void Ruler::SetActive(bool status)
{
  m_isActive = status;
  if (!m_isActive)
  {
    m_polyline.clear();
    m_sumDistanceM = 0.0;
  }
}

void Ruler::AddPoint(m2::PointD const & point)
{
  m_pointsPair[0] = m_pointsPair[1];
  m_pointsPair[1] = mercator::ToLatLon(point);
  m_polyline.insert(m_polyline.begin(), point);
  if (IsValidPolyline())
    SetDistance();
}

void Ruler::DrawLine(df::DrapeApi & drapeApi)
{
  if (!IsValidPolyline())
    return;

  static dp::Color const lightGreyColor = dp::Color(102, 102, 102, 210);

  drapeApi.RemoveLine(m_id);
  SetId();
  drapeApi.AddLine(m_id, df::DrapeApiLineData(m_polyline, lightGreyColor).Width(7.0f).ShowPoints(true).ShowId());
}

void Ruler::EraseLine(df::DrapeApi & drapeApi)
{
  drapeApi.RemoveLine(m_id);
}

bool Ruler::IsValidPolyline()
{
  return m_polyline.size() > 1;
}

void Ruler::SetDistance()
{
  m_sumDistanceM += oblate_spheroid::GetDistance(m_pointsPair[0], m_pointsPair[1]);
}

void Ruler::SetId()
{
  QLocale const locale;
  if (m_sumDistanceM >= 1000.0)
    m_id = (locale.toString(m_sumDistanceM / 1000.0, 'f', 1) + " " + Tr("km")).toStdString();
  else
    m_id = (locale.toString(m_sumDistanceM, 'f', 1) + " " + Tr("m")).toStdString();
}
}  // namespace qt
