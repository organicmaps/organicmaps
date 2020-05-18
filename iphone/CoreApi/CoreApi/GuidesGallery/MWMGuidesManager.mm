#import "MWMGuidesManager.h"

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
