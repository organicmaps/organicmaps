#import "CatalogPromoItem+Core.h"

@implementation CatalogPromoItem (Core)

- (instancetype)initWithCoreItem:(promo::CityGallery::Item const &)item {
  self = [super init];
  if (self) {
    self.placeTitle = @(item.m_place.m_name.c_str());
    self.placeDescription = @(item.m_place.m_description.c_str());
    self.imageUrl = @(item.m_imageUrl.c_str());
    self.catalogUrl = @(item.m_url.c_str());
    self.guideName = @(item.m_name.c_str());
    self.guideAuthor = @(item.m_author.m_name.c_str());
  }
  return self;
}

@end
