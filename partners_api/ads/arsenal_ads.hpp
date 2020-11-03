#pragma once

#include "partners_api/ads/ads_base.hpp"

namespace ads
{
class ArsenalMedic : public DownloadOnMapContainer
{
public:
  explicit  ArsenalMedic(Delegate & delegate);

private:
  std::string GetBannerInternal() const override;
};

class ArsenalFlat : public DownloadOnMapContainer
{
public:
  explicit  ArsenalFlat(Delegate & delegate);

private:
  std::string GetBannerInternal() const override;
};

class ArsenalInsuranceCrimea : public DownloadOnMapContainer
{
public:
  explicit  ArsenalInsuranceCrimea(Delegate & delegate);

private:
  std::string GetBannerInternal() const override;
};

class ArsenalInsuranceRussia : public DownloadOnMapContainer
{
public:
  explicit  ArsenalInsuranceRussia(Delegate & delegate);

private:
  std::string GetBannerInternal() const override;
};

class ArsenalInsuranceWorld : public DownloadOnMapContainer
{
public:
  explicit  ArsenalInsuranceWorld(Delegate & delegate);

private:
  std::string GetBannerInternal() const override;
};
}  // namespace ads

