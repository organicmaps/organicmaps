#include "map/nogps/storage.hpp"

namespace nogps
{
std::optional<std::string> SettingsStorage::GetString(std::string_view key) const
{
  std::string value;
  if (!settings::Get(key, value))
    return {};
  return value;
}

void SettingsStorage::SetString(std::string_view key, std::string const & value)
{
  settings::Set(key, value);
}
}  // namespace nogps
