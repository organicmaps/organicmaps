#import "MWMGuidesManager.h"
#import "GuidesGalleryItem+Core.h"

#include "Framework.h"

@interface MWMGuidesManager ()

@end

static GuidesManager & guidesManager() { return GetFramework().GetGuidesManager(); }

@implementation MWMGuidesManager

+ (instancetype)sharedManager {
  static MWMGuidesManager *manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[self alloc] init];
  });
  return manager;
}

- (NSArray<GuidesGalleryItem *> *)galleryItems {
  auto const &guidesGallery = guidesManager().GetGallery();
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
  return [itemsArray copy];
}

- (NSString *)activeGuideId {
  return @(guidesManager().GetActiveGuide().c_str());
}

- (void)setActiveGuide:(NSString *)guideId {
  guidesManager().SetActiveGuide(guideId.UTF8String);
}

- (void)setGalleryChangedCallback:(GalleryChangedBlock)callback {
  guidesManager().SetGalleryListener([callback] (bool reloadGallery) {
    callback(reloadGallery);
  });
}

- (void)resetGalleryChangedCallback {
  guidesManager().SetGalleryListener(nullptr);
}

@end
