#import "BookmarksCategoryLoadingResult.h"

#include "map/bookmark_manager.hpp"

NS_ASSUME_NONNULL_BEGIN

@interface BookmarksCategoryLoadingResult (Core)

- (instancetype)initWithCoreResult:(BookmarkManager::CategoryLoadingResult const &)result;

@end

NS_ASSUME_NONNULL_END
