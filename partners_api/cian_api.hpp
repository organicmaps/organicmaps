#pragma once

#include "partners_api/utils.hpp"

#include "geometry/latlon.hpp"
#include "geometry/rect2d.hpp"

#include "platform/safe_callback.hpp"

#include "base/worker_thread.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace cian
{
extern std::string const kBaseUrl;

class RawApi
{
public:
  static partners_api::http::Result GetRentNearby(m2::RectD const & rect,
                                                  std::string const & url = kBaseUrl);
};

struct RentOffer
{
  std::string m_flatType;
  uint8_t m_roomsCount = 0;
  double m_priceRur = 0.0;
  uint8_t m_floorNumber = 0;
  uint8_t m_floorsCount = 0;
  std::string m_url;
  std::string m_address;

  // No need to use offer when it is not fully filled.
  bool IsValid() const
  {
    return !m_flatType.empty() && m_roomsCount && m_priceRur != 0.0 && m_floorNumber &&
           m_floorsCount && !m_url.empty() && !m_address.empty();
  }
};

struct RentPlace
{
  ms::LatLon m_latlon;
  std::string m_url;
  std::vector<RentOffer> m_offers;
};

class Api
{
public:

  using RentNearbyCallback =
      platform::SafeCallback<void(std::vector<RentPlace> const & places, uint64_t const requestId)>;

  using ErrorCallback = platform::SafeCallback<void(int httpCode, uint64_t const requestId)>;

  explicit Api(std::string const & baseUrl = kBaseUrl);

  uint64_t GetRentNearby(ms::LatLon const & latlon, RentNearbyCallback const & onSuccess,
                         ErrorCallback const & onError);

  static bool IsCitySupported(std::string const & city);
  static std::string const & GetMainPageUrl();

private:
  uint64_t m_requestId = 0;
  std::string m_baseUrl;
};
}  // namespace cian
