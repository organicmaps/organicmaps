#include "user_mark.hpp"
#include "user_mark_container.hpp"

#include "../indexer/mercator.hpp"

UserMark::UserMark(m2::PointD const & ptOrg, UserMarkContainer * container)
  : m_ptOrg(ptOrg)
  , m_container(container)
{
}

UserMark::~UserMark() {}

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

void UserMark::Activate() const
{
  m_container->ActivateMark(this);
}

void UserMark::Diactivate() const
{
  m_container->DiactivateMark();
}
