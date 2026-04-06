#import "StringUtils.h"
#import "StringUtils+Core.h"

#include "indexer/search_string_utils.hpp"

@implementation NSString (StringUtils)

- (NSString *)normalizedAndSimplified
{
  auto const normalized = search::NormalizeAndSimplifyString(self.UTF8String);
  return [[NSString alloc] initWithBytes:normalized.data()
                                  length:normalized.size() * sizeof(strings::UniChar)
                                encoding:NSUTF32LittleEndianStringEncoding];
}

@end
