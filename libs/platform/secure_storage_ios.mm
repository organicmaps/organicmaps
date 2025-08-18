#include "platform/secure_storage.hpp"

#import <Foundation/Foundation.h>

namespace platform
{

NSString * StorageKey(std::string const & key)
{
  return [NSString stringWithFormat:@"Maps.me::PlatrormKey::%@", @(key.c_str())];
}

void SecureStorage::Save(std::string const & key, std::string const & value)
{
  [NSUserDefaults.standardUserDefaults setObject:@(value.c_str()) forKey:StorageKey(key)];
}

bool SecureStorage::Load(std::string const & key, std::string & value)
{
  NSString * val = [NSUserDefaults.standardUserDefaults objectForKey:StorageKey(key)];
  if (!val)
    return false;
  value = val.UTF8String;
  return true;
}

void SecureStorage::Remove(std::string const & key)
{
  [NSUserDefaults.standardUserDefaults removeObjectForKey:StorageKey(key)];
}
}  // namespace platform
