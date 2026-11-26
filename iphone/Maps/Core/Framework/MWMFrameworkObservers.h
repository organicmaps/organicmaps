#import "MWMFrameworkObserver.h"
#import "MWMRouterRecommendation.h"

#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"

#include "storage/storage.hpp"
#include "storage/storage_defines.hpp"

#include "platform/downloader_defines.hpp"

using namespace storage;

@protocol MWMFrameworkRouteBuilderObserver <MWMFrameworkObserver>

- (void)processRouteBuilderEvent:(routing::RouterResultCode)code
                       countries:(storage::CountriesSet const &)absentCountries;

@optional

- (void)processRouteBuilderProgress:(CGFloat)progress;
- (void)processRouteRecommendation:(MWMRouterRecommendation)recommendation;
- (void)speedCameraShowedUpOnRoute:(double)speedLimitKMph;
- (void)speedCameraLeftVisibleArea;
// A map tap or location update started a new track preview in the core.
- (void)processTrackPreviewBuildStarted;

@end

@protocol MWMFrameworkDrapeObserver <MWMFrameworkObserver>

@optional

- (void)processViewportCountryEvent:(storage::CountryId const &)countryId;
- (void)processViewportChangedEvent;

@end
