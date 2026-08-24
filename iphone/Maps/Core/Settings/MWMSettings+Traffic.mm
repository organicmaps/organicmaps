#import "MWMSettings+Traffic.h"

#include <CoreApi/Framework.h>

@implementation MWMSettings (Traffic)

+ (NSString *)trafficApiKey
{
  return @(Framework::GetTrafficApiKey().c_str());
}

+ (void)setTrafficApiKey:(NSString *)apiKey
{
  GetFramework().SetTrafficApiKey(apiKey.UTF8String);
}

+ (BOOL)isWellFormedTrafficApiKey:(NSString *)apiKey
{
  return Framework::IsWellFormedTrafficApiKey(apiKey.UTF8String);
}

@end
