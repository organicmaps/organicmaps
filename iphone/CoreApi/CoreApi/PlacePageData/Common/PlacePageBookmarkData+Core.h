#import "PlacePageBookmarkData.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageBookmarkData (Core)

- (instancetype)initWithRawData:(place_page::Info const &)rawData;
- (kml::ColorData)kmlColor;

@end

NS_ASSUME_NONNULL_END
