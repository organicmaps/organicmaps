#import "DeepLinkParser.h"
#include <CoreApi/Framework.h>
#import "DeepLinkSearchData.h"

#import "map/mwm_url.hpp"

static inline DeeplinkUrlType deeplinkUrlType(url_scheme::ParsedMapApi::UrlType type)
{
  switch (type)
   {
     case url_scheme::ParsedMapApi::UrlType::Incorrect: return DeeplinkUrlTypeIncorrect;
     case url_scheme::ParsedMapApi::UrlType::Map: return DeeplinkUrlTypeMap;
     case url_scheme::ParsedMapApi::UrlType::Route: return DeeplinkUrlTypeRoute;
     case url_scheme::ParsedMapApi::UrlType::Search: return DeeplinkUrlTypeSearch;
     case url_scheme::ParsedMapApi::UrlType::Crosshair: return DeeplinkUrlTypeCrosshair;
   }
}

@implementation DeepLinkParser

+ (DeeplinkUrlType)parseAndSetApiURL:(NSURL *)url {
  Framework &f = GetFramework();
  return deeplinkUrlType(f.ParseAndSetApiURL(url.absoluteString.UTF8String));
}

+ (void)executeMapApiRequest {
  GetFramework().ExecuteMapApiRequest();
}

+ (void)addBookmarksFile:(NSURL *)url {
  // iOS doesn't create temporary files on import at least in Safari and Files.
  GetFramework().AddBookmarksFile(url.path.UTF8String, false /* isTemporaryFile */);
}

@end
