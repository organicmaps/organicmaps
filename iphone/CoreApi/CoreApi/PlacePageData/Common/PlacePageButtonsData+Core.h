#import "PlacePageButtonsData.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageButtonsData (Core)

- (instancetype)initWithRawData:(place_page::Info const &)rawData;

@end

NS_ASSUME_NONNULL_END
