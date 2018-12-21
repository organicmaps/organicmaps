#pragma once

#include "map/bookmark_manager.hpp"
#include "map/local_ads_manager.hpp"
#include "map/notifications/notification_manager.hpp"
#include "map/user.hpp"

#include "ugc/storage.hpp"

#include "storage/country_info_reader_light.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <boost/optional.hpp>

namespace lightweight
{
struct LightFrameworkTest;

enum RequestType
{
  REQUEST_TYPE_EMPTY = 0u,
  REQUEST_TYPE_NUMBER_OF_UNSENT_UGC = 1u << 0,
  REQUEST_TYPE_USER_AUTH_STATUS = 1u << 1,
  REQUEST_TYPE_NUMBER_OF_UNSENT_EDITS = 1u << 2,
  REQUEST_TYPE_BOOKMARKS_CLOUD_ENABLED = 1u << 3,
  // Be careful to use this flag. Loading with this flag can produce a hard pressure on the disk
  // and takes much time.  For example it takes ~50ms on LG Nexus 5, ~100ms on Samsung A5, ~200ms on
  // Fly IQ4403.
  REQUEST_TYPE_LOCATION = 1u << 4,
  REQUEST_TYPE_LOCAL_ADS_FEATURES = 1u << 5,
  REQUEST_TYPE_LOCAL_ADS_STATISTICS = 1u << 6,
  REQUEST_TYPE_NOTIFICATION = 1u << 7,
};

using RequestTypeMask = unsigned;

// A class which allows you to acquire data in a synchronous way.
// The common use case is to create an instance of Framework
// with specified mask, acquire data according to the mask and destroy the instance.

class Framework
{
public:
  friend struct LightFrameworkTest;

  explicit Framework(RequestTypeMask request);

  bool IsUserAuthenticated() const;
  size_t GetNumberOfUnsentUGC() const;
  size_t GetNumberOfUnsentEdits() const;
  bool IsBookmarksCloudEnabled() const;
  CountryInfoReader::Info GetLocation(m2::PointD const & pt) const;
  std::vector<CampaignFeature> GetLocalAdsFeatures(double lat, double lon, double radiusInMeters,
                                                   uint32_t maxCount);
  Statistics * GetLocalAdsStatistics();
  boost::optional<notifications::NotificationCandidate> GetNotification() const;

private:
  RequestTypeMask m_request;
  bool m_userAuthStatus = false;
  size_t m_numberOfUnsentUGC = 0;
  size_t m_numberOfUnsentEdits = 0;
  bool m_bookmarksCloudEnabled = false;
  std::unique_ptr<CountryInfoReader> m_countryInfoReader;
  std::unique_ptr<LocalAdsFeaturesReader> m_localAdsFeaturesReader;
  std::unique_ptr<Statistics> m_localAdsStatistics;
  std::unique_ptr<lightweight::NotificationManager> m_notificationManager;
};

std::string FeatureParamsToString(int64_t mwmVersion, std::string const & countryId, uint32_t featureIndex);

bool FeatureParamsFromString(std::string const & str, int64_t & mwmVersion, std::string & countryId,
                             uint32_t & featureIndex);
}  // namespace lightweight
