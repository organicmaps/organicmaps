#import "PlacePageTrackData.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageTrackData (Core)

- (instancetype)initWithRawData:(place_page::Info const &)rawData
           onActivePointChanged:(MWMVoidBlock)onActivePointChangedHandler;

@end

NS_ASSUME_NONNULL_END
