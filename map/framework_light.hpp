#pragma once

#include "map/user.hpp"

#include "ugc/storage.hpp"

#include "base/assert.hpp"

namespace lightweight
{
struct LightFrameworkTest;

enum RequestType
{
  REQUEST_TYPE_EMPTY = 0u,
  REQUEST_TYPE_NUMBER_OF_UNSENT_UGC = 1u << 0,
  REQUEST_TYPE_USER_AUTH_STATUS = 1u << 1,
  REQUEST_TYPE_NUMBER_OF_UNSENT_EDITS = 1u << 2,
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

    CHECK_EQUAL(request, REQUEST_TYPE_EMPTY, ("Incorrect mask type:", request));
  }

  template <RequestTypeMask Type>
  auto Get() const;

private:
  RequestTypeMask m_request;
  bool m_userAuthStatus = false;
  size_t m_numberOfUnsentUGC = 0;
  size_t m_numberOfUnsentEdits = 0;
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
}  // namespace lightweight
