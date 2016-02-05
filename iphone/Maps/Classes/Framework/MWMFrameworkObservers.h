#include "map/user_mark.hpp"
#include "platform/location.hpp"
#include "routing/router.hpp"
#include "storage/index.hpp"
#include "storage/storage_defines.hpp"

@protocol MWMFrameworkObserver <NSObject>

@end

@protocol MWMFrameworkRouteBuilderObserver <MWMFrameworkObserver>

- (void)processRouteBuilderEvent:(routing::IRouter::ResultCode)code
                       countries:(storage::TCountriesVec const &)absentCountries
                          routes:(storage::TCountriesVec const &)absentRoutes;

@optional

- (void)processRouteBuilderProgress:(CGFloat)progress;

@end

@protocol MWMFrameworkMyPositionObserver <MWMFrameworkObserver>

- (void)processMyPositionStateModeChange:(location::EMyPositionMode)mode;

@end

@protocol MWMFrameworkUserMarkObserver <MWMFrameworkObserver>

- (void)processUserMarkActivation:(UserMark const *)mark;

@end

@protocol MWMFrameworkStorageObserver <MWMFrameworkObserver>

- (void)processCountryEvent:(storage::TCountryId const &)countryId;

@optional

- (void)processCountry:(storage::TCountryId const &)countryId progress:(storage::TLocalAndRemoteSize const &)progress;

@end

