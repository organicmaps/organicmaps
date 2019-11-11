#import "HotelRooms.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface HotelRooms (Core)

- (instancetype)initWithBlocks:(booking::Blocks const &)blocks;

@end

NS_ASSUME_NONNULL_END
