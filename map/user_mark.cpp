#include "user_mark.hpp"
#include "user_mark_container.hpp"

#include "../indexer/mercator.hpp"

namespace
{
  static EmptyCustomData s_emptyData;
}

UserMark::UserMark(const m2::PointD & ptOrg, UserMarkContainer * container)
  : m_ptOrg(ptOrg)
  , m_container(container)
  , m_customData(NULL)
{
}

UserMark::~UserMark()
{
  delete m_customData;
}

UserMarkContainer const * UserMark::GetContainer() const
{
  return m_container;
}

m2::PointD const & UserMark::GetOrg() const
{
  return m_ptOrg;
}

void UserMark::GetLatLon(double & lat, double & lon) const
{
  lon = MercatorBounds::XToLon(m_ptOrg.x);
  lat = MercatorBounds::YToLat(m_ptOrg.y);
}

void UserMark::InjectCustomData(UserCustomData * customData)
{
  delete m_customData;
  m_customData = customData;
}

UserCustomData const & UserMark::GetCustomData() const
{
  if (m_customData == NULL)
    return s_emptyData;

  return *m_customData;
}

void UserMark::Activate() const
{
  m_container->ActivateMark(this);
}

void UserMark::Diactivate() const
{
  m_container->DiactivateMark();
}

UserCustomData & UserMark::GetCustomData()
{
  if (m_customData == NULL)
    return s_emptyData;

  return *m_customData;
}
