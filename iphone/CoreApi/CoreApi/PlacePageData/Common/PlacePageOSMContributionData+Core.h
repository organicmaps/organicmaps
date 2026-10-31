#import "PlacePageOSMContributionData.h"

#include "map/place_page_info.hpp"

#include "storage/storage_defines.hpp"

@interface PlacePageOSMContributionData (Core)

/// The map's own status: terrain downloads and failures must not replace the editing controls.
- (instancetype _Nullable)initWithRawData:(place_page::Info const &)rawData mapStatus:(storage::Status)mapStatus;

@end
