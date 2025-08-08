#import "PlacePageOSMContributionData.h"

#include <CoreApi/Framework.h>

@class MWMMapNodeAttributes;

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageOSMContributionData (Core)

- (instancetype)initWithRawData:(place_page::Info const &)rawData mapAttributes:(MWMMapNodeAttributes * _Nullable)mapAttributes;

@end

NS_ASSUME_NONNULL_END
