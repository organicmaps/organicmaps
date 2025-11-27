#import "StringUtils.h"
#import "StringUtils+Core.h"

#include "indexer/search_string_utils.hpp"

@implementation NSString (StringUtils)

- (NSString *)normalizedAndSimplified
{
  return ToNSString(strings::ToUtf8(search::NormalizeAndSimplifyString(self.UTF8String)));
}

@end
