#import "GuidesGalleryData.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface GuidesGalleryData (Core)

- (instancetype)initWithGuidesGallery:(GuidesManager::GuidesGallery const &)guidesGallery;

@end

NS_ASSUME_NONNULL_END
