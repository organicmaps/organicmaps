#import <Foundation/Foundation.h>
#import "map/mwm_url.hpp"

NS_ASSUME_NONNULL_BEGIN

static inline DeeplinkParsingResult deeplinkParsingResult(url_scheme::ParsedMapApi::ParsingResult result)
{
  switch (result)
   {
     case url_scheme::ParsedMapApi::ParsingResult::Incorrect: return DeeplinkParsingResultIncorrect;
     case url_scheme::ParsedMapApi::ParsingResult::Map: return DeeplinkParsingResultMap;
     case url_scheme::ParsedMapApi::ParsingResult::Route: return DeeplinkParsingResultRoute;
     case url_scheme::ParsedMapApi::ParsingResult::Search: return DeeplinkParsingResultSearch;
     case url_scheme::ParsedMapApi::ParsingResult::Lead: return DeeplinkParsingResultLead;
     case url_scheme::ParsedMapApi::ParsingResult::Catalogue: return DeeplinkParsingResultCatalogue;
     case url_scheme::ParsedMapApi::ParsingResult::CataloguePath: return DeeplinkParsingResultCataloguePath;
     case url_scheme::ParsedMapApi::ParsingResult::Subscription: return DeeplinkParsingResultSubscription;
   }
}

NS_ASSUME_NONNULL_END
