#import "CatalogPromoItem.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface CatalogPromoItem (Core)

- (instancetype)initWithCoreItem:(promo::CityGallery::Item const &)item;

@end

NS_ASSUME_NONNULL_END
