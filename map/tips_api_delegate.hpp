#pragma once

#include "map/framework.hpp"

#include "map/tips_api.hpp"

class TipsApiDelegate : public TipsApi::Delegate
{
public:
  explicit TipsApiDelegate(Framework const & framework);

  std::optional<m2::PointD> GetCurrentPosition() const override;
  bool IsCountryLoaded(m2::PointD const & pt) const override;
  bool HaveTransit(m2::PointD const & pt) const override;
  double GetLastBackgroundTime() const override;
  m2::PointD const & GetViewportCenter() const override;
  storage::CountryId GetCountryId(m2::PointD const & pt) const override;
  isolines::Quality GetIsolinesQuality(storage::CountryId const & countryId) const override;

private:
  Framework const & m_framework;
};
