#import "DeepLinkSubscriptionData.h"
#import <CoreApi/Framework.h>

@implementation DeepLinkSubscriptionData

- (instancetype)init:(DeeplinkUrlType)urlType success:(BOOL)success {
  self = [super init];
  if (self) {
    _urlType = urlType;
    _success = success;
    auto const &request = GetFramework().GetParsedSubscription();
    _groups = @(request.m_groups.c_str());
  }
  return self;
}

@end
