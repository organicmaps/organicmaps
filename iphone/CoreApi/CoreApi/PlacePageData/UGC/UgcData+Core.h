#import "UgcData.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface UgcData (Core)

- initWithUgc:(ugc::UGC const &)ugc ugcUpdate:(ugc::UGCUpdate const &)update;

@end

NS_ASSUME_NONNULL_END
