#import "CatalogPromoItem.h"

#include "partners_api/promo_api.hpp"

NS_ASSUME_NONNULL_BEGIN

@interface CatalogPromoItem (Core)

- (instancetype)initWithCoreItem:(promo::CityGallery::Item const &)item;

@end

NS_ASSUME_NONNULL_END
