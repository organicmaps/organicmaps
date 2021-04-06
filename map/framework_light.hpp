#pragma once

#include "map/bookmark_manager.hpp"
#include "map/user.hpp"

#include "storage/country_info_reader_light.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace lightweight
{
struct LightFrameworkTest;

enum RequestType
{
  REQUEST_TYPE_EMPTY = 0u,
  REQUEST_TYPE_USER_AUTH_STATUS = 1u << 1,
  REQUEST_TYPE_NUMBER_OF_UNSENT_EDITS = 1u << 2,
  REQUEST_TYPE_BOOKMARKS_CLOUD_ENABLED = 1u << 3,
  // Be careful to use this flag. Loading with this flag can produce a hard pressure on the disk
  // and takes much time.  For example it takes ~50ms on LG Nexus 5, ~100ms on Samsung A5, ~200ms on
  // Fly IQ4403.
  REQUEST_TYPE_LOCATION = 1u << 4,
};

using RequestTypeMask = unsigned;

/** @brief A class which allows you to acquire data in a synchronous way.
 * The common use case is to create an instance of Framework
 * with specified mask, acquire data according to the mask and destroy the instance.
 * @note Seems like we can delete it, at first sight 'lightweight' used only in tracking,
 * but the class looks usefull itself (comparing with heavy ::Framework).
 */

class Framework
{
public:
  friend struct LightFrameworkTest;

  explicit Framework(RequestTypeMask request);

  bool IsUserAuthenticated() const;
  size_t GetNumberOfUnsentEdits() const;
  bool IsBookmarksCloudEnabled() const;

  /// @note Be careful here, because "lightweight" has no region's geometry cache.
  CountryInfoReader::Info GetLocation(m2::PointD const & pt) const;

private:
  RequestTypeMask m_request;
  bool m_userAuthStatus = false;
  size_t m_numberOfUnsentEdits = 0;
  bool m_bookmarksCloudEnabled = false;
  std::unique_ptr<CountryInfoReader> m_countryInfoReader;
};

std::string FeatureParamsToString(int64_t mwmVersion, std::string const & countryId, uint32_t featureIndex);

bool FeatureParamsFromString(std::string const & str, int64_t & mwmVersion, std::string & countryId,
                             uint32_t & featureIndex);
}  // namespace lightweight
