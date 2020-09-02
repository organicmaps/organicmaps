#import "DeepLinkParser.h"
#include <CoreApi/Framework.h>
#import "DeepLinkData.h"
#import "DeepLinkSearchData.h"
#import "DeepLinkSubscriptionData.h"
#import "DeeplinkUrlType.h"

@implementation DeepLinkParser

+ (id<IDeepLinkData>)parseAndSetApiURL:(NSURL *)url {
  Framework &f = GetFramework();
  url_scheme::ParsedMapApi::ParsingResult internalResult = f.ParseAndSetApiURL(url.absoluteString.UTF8String);
  DeeplinkUrlType result = deeplinkUrlType(internalResult.m_type);
  
  // TODO: remove this if-contition and implement correct result handling.
  // This condition is added for backward compatibility only.
  // Two states were represented by DeeplinkUrlTypeIncorrect: incorrect url and failed parsing.
  // But now it replaced by two parameters: url type and isSuccess flag.
  if (!internalResult.m_isSuccess)
   result = DeeplinkUrlTypeIncorrect;
   
  switch (result) {
    case DeeplinkUrlTypeSearch:
      return [[DeepLinkSearchData alloc] init:result];
    case DeeplinkUrlTypeSubscription:
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
