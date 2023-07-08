#include "platform/secure_storage.hpp"
#include "platform/settings.hpp"

namespace platform
{
void SecureStorage::Save(std::string const & key, std::string const & value)
{
  settings::Set(key, value);
}

bool SecureStorage::Load(std::string const & key, std::string & value)
{
  bool const result = settings::Get(key, value);
  return result && !value.empty();
}

void SecureStorage::Remove(std::string const & key)
{
  settings::Set(key, std::string());
}
}  // namespace platform
