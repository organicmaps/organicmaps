#import "DeepLinkSubscriptionData.h"
#import <CoreApi/Framework.h>

@implementation DeepLinkSubscriptionData

- (instancetype)init:(DeeplinkParsingResult)result {
  self = [super init];
  if (self) {
    _result = result;
    auto const &request = GetFramework().GetParsedSubscription();
    _deliverable = @(request.m_deliverable.c_str());
  }
  return self;
}

@end
