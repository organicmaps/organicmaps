#import "CatalogPromoItem+Core.h"

@implementation CatalogPromoItem

@end

@implementation CatalogPromoItem (Core)

- (instancetype)initWithCoreItem:(promo::CityGallery::Item const &)item {
  self = [super init];
  if (self) {
    _placeTitle = @(item.m_place.m_name.c_str());
    _placeDescription = @(item.m_place.m_description.c_str());
    _imageUrl = @(item.m_imageUrl.c_str());
    _catalogUrl = @(item.m_url.c_str());
    _guideName = @(item.m_name.c_str());
    _guideAuthor = @(item.m_author.m_name.c_str());
    _categoryLabel = @(item.m_luxCategory.m_name.c_str());
    _hexColor = @(item.m_luxCategory.m_color.c_str());
  }
  return self;
}

@end

