#include "partners_api/promo_api.hpp"

NS_ASSUME_NONNULL_BEGIN

@interface MWMDiscoveryCityGalleryObjects : NSObject

- (instancetype)initWithGalleryResults:(promo::CityGallery const &)results;
- (promo::CityGallery::Item const &)galleryItemAtIndex:(NSUInteger)index;
- (NSUInteger)count;
- (nullable NSURL *)moreURL;
- (NSString *)tagString;

@end

NS_ASSUME_NONNULL_END
