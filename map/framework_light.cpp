#include "map/framework_light.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <sstream>
#include <utility>

namespace
{
char const * kDelimiter = ":";
}  // namespace

namespace lightweight
{
Framework::Framework(RequestTypeMask request) : m_request(request)
{
  CHECK_NOT_EQUAL(request, REQUEST_TYPE_EMPTY, ("Mask is empty"));

  if (request & REQUEST_TYPE_USER_AUTH_STATUS)
  {
    m_userAuthStatus = impl::IsUserAuthenticated();
    request ^= REQUEST_TYPE_USER_AUTH_STATUS;
  }

  if (request & REQUEST_TYPE_NUMBER_OF_UNSENT_EDITS)
  {
    // TODO: Hasn't implemented yet.
    request ^= REQUEST_TYPE_NUMBER_OF_UNSENT_EDITS;
  }

  if (request & REQUEST_TYPE_BOOKMARKS_CLOUD_ENABLED)
  {
    m_bookmarksCloudEnabled = impl::IsBookmarksCloudEnabled();
    request ^= REQUEST_TYPE_BOOKMARKS_CLOUD_ENABLED;
  }

  if (request & REQUEST_TYPE_LOCATION)
  {
    m_countryInfoReader = std::make_unique<CountryInfoReader>();
    request ^= REQUEST_TYPE_LOCATION;
  }

  CHECK_EQUAL(request, REQUEST_TYPE_EMPTY, ("Incorrect mask type:", request));
}

bool Framework::IsUserAuthenticated() const
{
  ASSERT(m_request & REQUEST_TYPE_USER_AUTH_STATUS, (m_request));
  return m_userAuthStatus;
}

size_t Framework::GetNumberOfUnsentEdits() const
{
  ASSERT(m_request & REQUEST_TYPE_NUMBER_OF_UNSENT_EDITS, (m_request));
  return m_numberOfUnsentEdits;
}

bool Framework::IsBookmarksCloudEnabled() const
{
  ASSERT(m_request & REQUEST_TYPE_BOOKMARKS_CLOUD_ENABLED, (m_request));
  return m_bookmarksCloudEnabled;
}

CountryInfoReader::Info Framework::GetLocation(m2::PointD const & pt) const
{
  ASSERT(m_request & REQUEST_TYPE_LOCATION, (m_request));
  CHECK(m_countryInfoReader, ());

  return m_countryInfoReader->GetMwmInfo(pt);
}

std::string FeatureParamsToString(int64_t mwmVersion, std::string const & countryId, uint32_t featureIndex)
{
  std::ostringstream stream;
  stream << mwmVersion << kDelimiter << countryId << kDelimiter << featureIndex;
  return stream.str();
}

bool FeatureParamsFromString(std::string const & str, int64_t & mwmVersion, std::string & countryId,
                             uint32_t & featureIndex)
{
  std::vector<std::string> tokens;
  strings::Tokenize(str, kDelimiter, base::MakeBackInsertFunctor(tokens));
  if (tokens.size() != 3)
    return false;

  int64_t tmpMwmVersion;
  if (!strings::to_int64(tokens[0], tmpMwmVersion))
    return false;

  unsigned int tmpFeatureIndex;
  if (!strings::to_uint(tokens[2], tmpFeatureIndex))
    return false;

  if (tokens[1].empty())
    return false;

  mwmVersion = tmpMwmVersion;
  countryId = tokens[1];
  featureIndex = tmpFeatureIndex;

  return true;
}
}  // namespace lightweight
