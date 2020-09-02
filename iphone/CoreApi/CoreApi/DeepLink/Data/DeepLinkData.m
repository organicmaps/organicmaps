#import "DeepLinkData.h"

@implementation DeepLinkData

- (instancetype)init:(DeeplinkUrlType)urlType {
  self = [super init];
  if (self) {
    _urlType = urlType;
  }
  return self;
}

@end
