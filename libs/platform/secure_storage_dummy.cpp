#include "platform/secure_storage.hpp"

namespace platform
{
void SecureStorage::Save(std::string const & key, std::string const & value)
{
  // Unimplemented on this platform.
}

bool SecureStorage::Load(std::string const & key, std::string & value)
{
  // Unimplemented on this platform.
  return false;
}

void SecureStorage::Remove(std::string const & key)
{
  // Unimplemented on this platform.
}
}  // namespace platform
