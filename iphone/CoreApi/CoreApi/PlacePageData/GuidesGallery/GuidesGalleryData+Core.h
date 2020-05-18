#import "GuidesGalleryData.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface GuidesGalleryData (Core)

- (instancetype)initWithGuidesGallery:(GuidesManager::GuidesGallery const &)guidesGallery
                        activeGuideId:(NSString *)activeGuideId;

@end

NS_ASSUME_NONNULL_END
