#import "GuidesGalleryItem+Core.h"

@implementation GuidesGalleryItem

@end

@implementation GuidesGalleryItem (Core)

- (instancetype)initWithGuidesGalleryItem:(GuidesManager::GuidesGallery::Item const &)guidesGalleryItem {
  self = [super init];
  if (self) {
    _guideId = @(guidesGalleryItem.m_guideId.c_str());
    _url = @(guidesGalleryItem.m_url.c_str());
    _imageUrl = @(guidesGalleryItem.m_imageUrl.c_str());
    _title = @(guidesGalleryItem.m_title.c_str());
    _downloaded = guidesGalleryItem.m_downloaded;
  }
  return self;
}

@end

@implementation CityGalleryItem

@end

@implementation CityGalleryItem (Core)

- (instancetype)initWithGuidesGalleryItem:(GuidesManager::GuidesGallery::Item const &)guidesGalleryItem {
  self = [super initWithGuidesGalleryItem:guidesGalleryItem];
  if (self) {
    _bookmarksCount = guidesGalleryItem.m_cityParams.m_bookmarksCount;
    _hasTrack = guidesGalleryItem.m_cityParams.m_trackIsAvailable;
  }
  return self;
}

@end

@implementation OutdoorGalleryItem

@end

@implementation OutdoorGalleryItem (Core)

- (instancetype)initWithGuidesGalleryItem:(GuidesManager::GuidesGallery::Item const &)guidesGalleryItem {
  self = [super initWithGuidesGalleryItem:guidesGalleryItem];
  if (self) {
    _tag = @(guidesGalleryItem.m_outdoorsParams.m_tag.c_str());
    _distance = guidesGalleryItem.m_outdoorsParams.m_distance;
    _duration = guidesGalleryItem.m_outdoorsParams.m_duration;
    _ascent = guidesGalleryItem.m_outdoorsParams.m_ascent;
  }
  return self;
}

@end
