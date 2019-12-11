#import "CoreBanner.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface CoreBanner (Core)

- (instancetype)initWithAdBanner:(ads::Banner const &)banner;

@end

NS_ASSUME_NONNULL_END
