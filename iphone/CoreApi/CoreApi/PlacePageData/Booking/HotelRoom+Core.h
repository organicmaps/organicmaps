#import "HotelRoom.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface HotelRoom (Core)

- (instancetype)initWithBlockInfo:(booking::BlockInfo const &)blockInfo;

@end

NS_ASSUME_NONNULL_END
