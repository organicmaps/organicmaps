#pragma once

#include "geometry/latlon.hpp"
#include "geometry/rect2d.hpp"

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
  static bool GetRentNearby(m2::RectD const & rect, std::string & result,
                            std::string const & url = kBaseUrl);
};

struct RentOffer
{
  std::string m_flatType;
  uint8_t m_roomsCount = 0;
  uint32_t m_priceRur = 0;
  uint8_t m_floorNumber = 0;
  uint8_t m_floorsCount = 0;
  std::string m_url;
  std::string m_address;
};

struct RentPlace
{
  ms::LatLon m_latlon;
  std::vector<RentOffer> m_offers;
};

class Api
{
public:
  using RentNearbyCallback =
      std::function<void(std::vector<RentPlace> const & places, uint64_t const requestId)>;

  explicit Api(std::string const & baseUrl = kBaseUrl);
  virtual ~Api();

  uint64_t GetRentNearby(m2::RectD const & rect, RentNearbyCallback const & cb);

private:
  uint64_t m_requestId = 0;
  std::string m_baseUrl;
  base::WorkerThread m_worker;
};
}  // namespace cian
