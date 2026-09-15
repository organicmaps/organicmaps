#import "MWMBookmarksImportResult.h"

#include "map/bookmark_manager.hpp"

NS_ASSUME_NONNULL_BEGIN

@interface MWMBookmarksImportResult (Core)

- (instancetype)initWithCoreResult:(BookmarkManager::BookmarkImportResult const &)result;

@end

NS_ASSUME_NONNULL_END
