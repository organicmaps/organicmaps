#import "MWMDiscoveryCityGalleryObjects.h"

@interface MWMDiscoveryCityGalleryObjects() {
  promo::CityGallery m_results;
}
@end

@implementation MWMDiscoveryCityGalleryObjects

- (instancetype)initWithGalleryResults:(promo::CityGallery const &)results {
  self = [super init];
  if (self) {
    m_results = results;
  }
  return self;
}

- (promo::CityGallery::Item const &)galleryItemAtIndex:(NSUInteger)index {
  CHECK_LESS(index, m_results.m_items.size(), ("Incorrect index:", index));
  return m_results.m_items[index];
}

- (NSUInteger)count {
  return m_results.m_items.size();
}

- (NSURL *)moreURL {
  NSString *path = @(m_results.m_moreUrl.c_str());
  if (path != nil && path.length > 0) {
    return [NSURL URLWithString:path];
  }
  return nil;
}

- (NSString *)tagString {
  return @(m_results.m_category.c_str());
}

@end
