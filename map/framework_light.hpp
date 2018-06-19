#pragma once

#include "map/bookmark_manager.hpp"
#include "map/user.hpp"

#include "ugc/storage.hpp"

#include "storage/country_info_reader_light.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <memory>

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
};

using RequestTypeMask = unsigned;

// A class which allows you to acquire data in a synchronous way.
// The common use case is to create an instance of Framework
// with specified mask, acquire data according to the mask and destroy the instance.

class Framework
{
public:
  friend struct LightFrameworkTest;

  explicit Framework(RequestTypeMask request) : m_request(request)
  {
    CHECK_NOT_EQUAL(request, REQUEST_TYPE_EMPTY, ("Mask is empty"));

    if (request & REQUEST_TYPE_NUMBER_OF_UNSENT_UGC)
    {
      m_numberOfUnsentUGC = GetNumberOfUnsentUGC();
      request ^= REQUEST_TYPE_NUMBER_OF_UNSENT_UGC;
    }

    if (request & REQUEST_TYPE_USER_AUTH_STATUS)
    {
      m_userAuthStatus = IsUserAuthenticated();
      request ^= REQUEST_TYPE_USER_AUTH_STATUS;
    }

    if (request & REQUEST_TYPE_NUMBER_OF_UNSENT_EDITS)
    {
      // TODO: Hasn't implemented yet.
      request ^= REQUEST_TYPE_NUMBER_OF_UNSENT_EDITS;
    }

    if (request & REQUEST_TYPE_BOOKMARKS_CLOUD_ENABLED)
    {
      m_bookmarksCloudEnabled = IsBookmarksCloudEnabled();
      request ^= REQUEST_TYPE_BOOKMARKS_CLOUD_ENABLED;
    }

    if (request & REQUEST_TYPE_LOCATION)
    {
      m_countryInfoReader = std::make_unique<CountryInfoReader>();
      request ^= REQUEST_TYPE_LOCATION;
    }

    CHECK_EQUAL(request, REQUEST_TYPE_EMPTY, ("Incorrect mask type:", request));
  }

  template <RequestTypeMask Type>
  auto Get() const;

  template <RequestTypeMask Type>
  auto Get(m2::PointD const & pt) const;

private:
  RequestTypeMask m_request;
  bool m_userAuthStatus = false;
  size_t m_numberOfUnsentUGC = 0;
  size_t m_numberOfUnsentEdits = 0;
  bool m_bookmarksCloudEnabled = false;
  std::unique_ptr<CountryInfoReader> m_countryInfoReader;
};

template<>
auto Framework::Get<REQUEST_TYPE_USER_AUTH_STATUS>() const
{
  ASSERT(m_request & REQUEST_TYPE_USER_AUTH_STATUS, (m_request));
  return m_userAuthStatus;
}

template<>
auto Framework::Get<REQUEST_TYPE_NUMBER_OF_UNSENT_UGC>() const
{
  ASSERT(m_request & REQUEST_TYPE_NUMBER_OF_UNSENT_UGC, (m_request));
  return m_numberOfUnsentUGC;
}

template<>
auto Framework::Get<REQUEST_TYPE_NUMBER_OF_UNSENT_EDITS>() const
{
  ASSERT(m_request & REQUEST_TYPE_NUMBER_OF_UNSENT_EDITS, (m_request));
  return m_numberOfUnsentEdits;
}

template<>
auto Framework::Get<REQUEST_TYPE_BOOKMARKS_CLOUD_ENABLED>() const
{
  ASSERT(m_request & REQUEST_TYPE_BOOKMARKS_CLOUD_ENABLED, (m_request));
  return m_bookmarksCloudEnabled;
}

template <>
auto Framework::Get<REQUEST_TYPE_LOCATION>(m2::PointD const & pt) const
{
  ASSERT(m_request & REQUEST_TYPE_LOCATION, (m_request));

  CHECK(m_countryInfoReader, ());

  return m_countryInfoReader->GetMwmInfo(pt);
}
}  // namespace lightweight
