#include "map/tips_api_delegate.hpp"

TipsApiDelegate::TipsApiDelegate(Framework & framework)
  : m_framework(framework)
{
}

boost::optional<m2::PointD> TipsApiDelegate::GetCurrentPosition() const
{
  return m_framework.GetCurrentPosition();
}

bool TipsApiDelegate::IsCountryLoaded(m2::PointD const & pt) const
{
  return m_framework.IsCountryLoaded(pt);
}

bool TipsApiDelegate::HaveTransit(m2::PointD const & pt) const
{
  return m_framework.HaveTransit(pt);
}

double TipsApiDelegate::GetLastBackgroundTime() const
{
  return m_framework.GetLastBackgroundTime();
}

m2::PointD const & TipsApiDelegate::GetViewportCenter() const
{
  return m_framework.GetViewportCenter();
}

storage::CountryId TipsApiDelegate::GetCountryId(m2::PointD const & pt) const
{
  return m_framework.GetCountryIndex(pt);
}

int64_t TipsApiDelegate::GetCountryVersion(storage::CountryId const & countryId) const
{
  return m_framework.GetStorage().GetVersion(countryId);
}
