#import "DeepLinkData.h"

@implementation DeepLinkData

- (instancetype)init:(DeeplinkParsingResult)result {
  self = [super init];
  if (self) {
    _result = result;
  }
  return self;
}

@end
