#include "platform/http_user_agent.hpp"

#import <Foundation/Foundation.h>

namespace platform
{
std::string HttpUserAgent::ExtractAppVersion() const
{
  NSString * str = NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"];
  return std::string(str.UTF8String);
}
}  // platform
