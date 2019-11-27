#import "DeepLinkSubscriptionData.h"
#import <CoreApi/Framework.h>

@implementation DeepLinkSubscriptionData

- (instancetype)init:(DeeplinkParsingResult)result {
  self = [super init];
  if (self) {
    _result = result;
    auto const &request = GetFramework().GetParsedSubscription();
    _groups = @(request.m_groups.c_str());
  }
  return self;
}

@end
