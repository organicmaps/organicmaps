#import "PlacePageOSMContributionData.h"

#include "map/place_page_info.hpp"

@interface PlacePageOSMContributionData (Core)

- (instancetype _Nullable)initWithRawData:(place_page::Info const &)rawData;

@end
