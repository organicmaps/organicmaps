#import "StringUtils.h"
#import "StringUtils+Core.h"

#include "indexer/search_string_utils.hpp"

@implementation NSString (StringUtils)

- (NSString *)normalizedAndSimplified
{
  auto const u32str = search::NormalizeAndSimplifyString(self.UTF8String);
  return [[NSString alloc] initWithBytes:u32str.data()
                                  length:u32str.size() * sizeof(UniChar)
                                encoding:NSUTF32StringEncoding];
}

@end
