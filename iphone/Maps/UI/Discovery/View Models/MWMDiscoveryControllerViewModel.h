#include "map/discovery/discovery_client_params.hpp"

NS_ASSUME_NONNULL_BEGIN

@class MWMDiscoveryMapObjects;
@class MWMDiscoveryCityGalleryObjects;
@class MWMDiscoverySearchViewModel;
@class MWMDiscoveryHotelViewModel;
@class MWMDiscoveryGuideViewModel;

@interface MWMDiscoveryControllerViewModel : NSObject

@property(nonatomic, readonly) MWMDiscoveryMapObjects *attractions;
@property(nonatomic, readonly) MWMDiscoveryMapObjects *cafes;
@property(nonatomic, readonly) MWMDiscoveryMapObjects *hotels;
@property(nonatomic, readonly) MWMDiscoveryCityGalleryObjects *guides;

- (void)updateMapObjects:(MWMDiscoveryMapObjects *)objects
                 forType:(discovery::ItemType const)type;
- (void)updateCityGalleryObjects:(MWMDiscoveryCityGalleryObjects *)objects;
- (NSUInteger)itemsCountForType:(discovery::ItemType const)type;

- (MWMDiscoverySearchViewModel *)attractionAtIndex:(NSUInteger)index;
- (MWMDiscoverySearchViewModel *)cafeAtIndex:(NSUInteger)index;
- (MWMDiscoveryHotelViewModel *)hotelAtIndex:(NSUInteger)index;
- (MWMDiscoveryGuideViewModel *)guideAtIndex:(NSUInteger)index;

@end

NS_ASSUME_NONNULL_END
