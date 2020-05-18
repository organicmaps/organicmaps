#import "GuidesGalleryData+Core.h"
#import "GuidesGalleryItem+Core.h"

@implementation GuidesGalleryData

@end

@implementation GuidesGalleryData (Core)

- (instancetype)initWithGuidesGallery:(GuidesManager::GuidesGallery const &)guidesGallery
                        activeGuideId:(NSString *)activeGuideId {
  self = [super init];
  if (self) {
    NSMutableArray *itemsArray = [NSMutableArray arrayWithCapacity:guidesGallery.m_items.size()];
    for (auto const &item : guidesGallery.m_items) {
      GuidesGalleryItem *galleryItem;
      switch (item.m_type) {
        case GuidesManager::GuidesGallery::Item::Type::City:
          galleryItem = [[CityGalleryItem alloc] initWithGuidesGalleryItem:item];
          break;
        case GuidesManager::GuidesGallery::Item::Type::Outdoor:
          galleryItem = [[OutdoorGalleryItem alloc] initWithGuidesGalleryItem:item];
          break;
      }
      [itemsArray addObject:galleryItem];
    }
    _galleryItems = [itemsArray copy];
    _activeGuideId = [activeGuideId copy];
  }
  return self;
}

@end
