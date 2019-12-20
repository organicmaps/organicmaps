#import "MWMMapUpdateInfo.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface MWMMapUpdateInfo (Core)

- (instancetype)initWithUpdateInfo:(storage::Storage::UpdateInfo const &)updateInfo;

@end

NS_ASSUME_NONNULL_END
