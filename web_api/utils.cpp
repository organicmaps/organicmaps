#include "web_api/utils.hpp"

#include "platform/platform.hpp"

#include "coding/sha1.hpp"

namespace web_api
{
std::string DeviceId()
{
  return coding::SHA1::CalculateBase64ForString(GetPlatform().UniqueClientId());
}
}  // namespace web_api
