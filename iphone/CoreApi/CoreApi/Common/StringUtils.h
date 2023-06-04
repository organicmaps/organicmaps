#pragma once
#import <Foundation/Foundation.h>

#include <string_view>

inline NSString * ToNSString(std::string_view sv)
{
  return [[NSString alloc] initWithBytes:sv.data() length:sv.size() encoding:NSUTF8StringEncoding];
}
