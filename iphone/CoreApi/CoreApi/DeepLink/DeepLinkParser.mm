#import "DeepLinkParser.h"
#include <CoreApi/Framework.h>
#import "DeepLinkSearchData.h"

#import "map/mwm_url.hpp"

static inline DeeplinkUrlType deeplinkUrlType(url_scheme::ParsedMapApi::UrlType type)
{
  using namespace url_scheme;
  switch (type)
  {
  case ParsedMapApi::UrlType::Incorrect: return DeeplinkUrlTypeIncorrect;
  case ParsedMapApi::UrlType::Map: return DeeplinkUrlTypeMap;
  case ParsedMapApi::UrlType::Route: return DeeplinkUrlTypeRoute;
  case ParsedMapApi::UrlType::Search: return DeeplinkUrlTypeSearch;
  case ParsedMapApi::UrlType::Crosshair: return DeeplinkUrlTypeCrosshair;
  case ParsedMapApi::UrlType::OAuth2: return DeeplinkUrlTypeOAuth2;
  case ParsedMapApi::UrlType::Menu: return DeeplinkUrlTypeMenu;
  case ParsedMapApi::UrlType::Settings: return DeeplinkUrlTypeSettings;
  }
}

@implementation DeepLinkParser

+ (DeeplinkUrlType)parseAndSetApiURL:(NSURL *)url
{
  Framework & f = GetFramework();
  return deeplinkUrlType(f.ParseAndSetApiURL(url.absoluteString.UTF8String));
}

+ (void)executeMapApiRequest
{
  GetFramework().ExecuteMapApiRequest();
}

+ (void)addBookmarksFile:(NSURL *)url
{
  // iOS doesn't create temporary files on import at least in Safari and Files.
  GetFramework().AddBookmarksFile(url.path.UTF8String, false /* isTemporaryFile */);
}

@end
