#import "DeepLinkData.h"

@implementation DeepLinkData

- (instancetype)init:(DeeplinkUrlType)urlType success:(BOOL)success {
  self = [super init];
  if (self) {
    _urlType = urlType;
    _success = success;
  }
  return self;
}

@end
