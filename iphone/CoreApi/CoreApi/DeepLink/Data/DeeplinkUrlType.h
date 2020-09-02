#import <Foundation/Foundation.h>
#import "map/mwm_url.hpp"

NS_ASSUME_NONNULL_BEGIN

static inline DeeplinkUrlType deeplinkUrlType(url_scheme::ParsedMapApi::UrlType type)
{
  switch (type)
   {
     case url_scheme::ParsedMapApi::UrlType::Incorrect: return DeeplinkUrlTypeIncorrect;
     case url_scheme::ParsedMapApi::UrlType::Map: return DeeplinkUrlTypeMap;
     case url_scheme::ParsedMapApi::UrlType::Route: return DeeplinkUrlTypeRoute;
     case url_scheme::ParsedMapApi::UrlType::Search: return DeeplinkUrlTypeSearch;
     case url_scheme::ParsedMapApi::UrlType::Lead: return DeeplinkUrlTypeLead;
     case url_scheme::ParsedMapApi::UrlType::Catalogue: return DeeplinkUrlTypeCatalogue;
     case url_scheme::ParsedMapApi::UrlType::CataloguePath: return DeeplinkUrlTypeCataloguePath;
     case url_scheme::ParsedMapApi::UrlType::Subscription: return DeeplinkUrlTypeSubscription;
   }
}

NS_ASSUME_NONNULL_END
