#import "MWMRouterRecommendation.h"

#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"
#include "storage/storage.hpp"
#include "storage/storage_defines.hpp"

using namespace storage;

@protocol MWMFrameworkObserver<NSObject>

@end

@protocol MWMFrameworkRouteBuilderObserver<MWMFrameworkObserver>

- (void)processRouteBuilderEvent:(routing::RouterResultCode)code
                       countries:(storage::CountriesVec const &)absentCountries;

@optional

- (void)processRouteBuilderProgress:(CGFloat)progress;
- (void)processRouteRecommendation:(MWMRouterRecommendation)recommendation;

@end

@protocol MWMFrameworkStorageObserver<MWMFrameworkObserver>

- (void)processCountryEvent:(CountryId const &)countryId;

@optional

- (void)processCountry:(CountryId const &)countryId
              progress:(MapFilesDownloader::Progress const &)progress;

@end

@protocol MWMFrameworkDrapeObserver<MWMFrameworkObserver>

@optional

- (void)processViewportCountryEvent:(CountryId const &)countryId;
- (void)processViewportChangedEvent;

@end
