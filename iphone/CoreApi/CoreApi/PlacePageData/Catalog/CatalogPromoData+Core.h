#import "CatalogPromoData.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface CatalogPromoData (Core)

- (instancetype)initWithCityGallery:(promo::CityGallery const &)cityGallery;

@end

NS_ASSUME_NONNULL_END
