#import "PlacePageOSMContributionData.h"

#include <CoreApi/Framework.h>

@class MWMMapNodeAttributes;

@interface PlacePageOSMContributionData (Core)

- (instancetype _Nullable)initWithRawData:(place_page::Info const &)rawData mapAttributes:(MWMMapNodeAttributes * _Nonnull)mapAttributes;

@end
