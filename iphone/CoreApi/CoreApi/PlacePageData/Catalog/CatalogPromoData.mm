#import "CatalogPromoData+Core.h"

#import "CatalogPromoItem+Core.h"

@implementation CatalogPromoData

@end

@implementation CatalogPromoData (Core)

- (instancetype)initWithCityGallery:(promo::CityGallery const &)cityGallery {
  self = [super init];
  if (self) {
    _tagsString = @(cityGallery.m_category.c_str());
    NSString *urlString = @(cityGallery.m_moreUrl.c_str());
    if (urlString.length > 0) {
      _moreUrl = [NSURL URLWithString:urlString];
    }

    NSMutableArray *itemsArray = [NSMutableArray arrayWithCapacity:cityGallery.m_items.size()];
    for (auto const &item : cityGallery.m_items) {
      CatalogPromoItem *promoItem = [[CatalogPromoItem alloc] initWithCoreItem:item];
      [itemsArray addObject:promoItem];
    }
    _promoItems = [itemsArray copy];
  }
  return self;
}

@end
