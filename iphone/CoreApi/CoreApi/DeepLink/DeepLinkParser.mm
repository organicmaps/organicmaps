#import "DeepLinkParser.h"
#include <CoreApi/Framework.h>
#import "DeepLinkData.h"
#import "DeepLinkSearchData.h"
#import "DeeplinkUrlType.h"

@implementation DeepLinkParser

+ (id<IDeepLinkData>)parseAndSetApiURL:(NSURL *)url {
  Framework &f = GetFramework();
  url_scheme::ParsedMapApi::ParsingResult internalResult = f.ParseAndSetApiURL(url.absoluteString.UTF8String);
  DeeplinkUrlType result = deeplinkUrlType(internalResult.m_type);

  switch (result) {
    case DeeplinkUrlTypeSearch:
      return [[DeepLinkSearchData alloc] init:result success:internalResult.m_isSuccess];
    default:
      return [[DeepLinkData alloc] init:result success:internalResult.m_isSuccess];
  }
}

+ (bool)showMapForUrl:(NSURL *)url {
  return GetFramework().ShowMapForURL(url.absoluteString.UTF8String);
}

+ (void)addBookmarksFile:(NSURL *)url {
  GetFramework().AddBookmarksFile(url.relativePath.UTF8String, false /* isTemporaryFile */);
}

@end
