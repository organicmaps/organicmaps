#include "routing/router.hpp"
#include "storage/index.hpp"
#include "storage/storage.hpp"

using namespace storage;

@protocol MWMFrameworkObserver<NSObject>

@end

@protocol MWMFrameworkRouteBuilderObserver<MWMFrameworkObserver>

- (void)processRouteBuilderEvent:(routing::IRouter::ResultCode)code
                       countries:(storage::TCountriesVec const &)absentCountries;

@optional

- (void)processRouteBuilderProgress:(CGFloat)progress;

@end

@protocol MWMFrameworkStorageObserver<MWMFrameworkObserver>

- (void)processCountryEvent:(TCountryId const &)countryId;

@optional

- (void)processCountry:(TCountryId const &)countryId
              progress:(MapFilesDownloader::TProgress const &)progress;

@end

@protocol MWMFrameworkDrapeObserver<MWMFrameworkObserver>

- (void)processViewportCountryEvent:(TCountryId const &)countryId;

@end
