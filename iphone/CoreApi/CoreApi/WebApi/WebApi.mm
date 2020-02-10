#import <WebApi.h>

#include "web_api/request_headers.hpp"

@implementation WebApi

+ (NSDictionary<NSString *, NSString *> *)getDefaultAuthHeaders {
  NSMutableDictionary<NSString *, NSString *> *result = [NSMutableDictionary dictionary];

  for (auto const &header : web_api::GetDefaultAuthHeaders())
    [result setObject:@(header.second.c_str()) forKey:@(header.first.c_str())];

  return [result copy];
}

@end
