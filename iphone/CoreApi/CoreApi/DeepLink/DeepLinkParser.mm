#import "DeepLinkParser.h"
#include <CoreApi/Framework.h>
#import "DeepLinkData.h"
#import "DeepLinkSearchData.h"
#import "DeepLinkSubscriptionData.h"
#import "DeeplinkParsingResult.h"

@implementation DeepLinkParser

+ (id<IDeepLinkData>)parseAndSetApiURL:(NSURL *)url {
  Framework &f = GetFramework();
  DeeplinkParsingResult result = deeplinkParsingResult(f.ParseAndSetApiURL(url.absoluteString.UTF8String));
  switch (result) {
    case DeeplinkParsingResultSearch:
      return [[DeepLinkSearchData alloc] init:result];
    case DeeplinkParsingResultSubscription:
      return [[DeepLinkSubscriptionData alloc] init:result];
    default:
      return [[DeepLinkData alloc] init:result];
  }
}

+ (bool)showMapForUrl:(NSURL *)url {
  return GetFramework().ShowMapForURL(url.absoluteString.UTF8String);
}

+ (void)addBookmarksFile:(NSURL *)url {
  GetFramework().AddBookmarksFile(url.relativePath.UTF8String, false /* isTemporaryFile */);
}

@end
