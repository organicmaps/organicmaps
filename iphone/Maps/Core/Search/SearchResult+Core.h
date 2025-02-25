#import "SearchResult.h"

#import "search/result.hpp"

NS_ASSUME_NONNULL_BEGIN

@interface SearchResult (Core)

- (instancetype)initWithResult:(const search::Result &)result itemType:(SearchItemType)itemType index:(NSUInteger)index;

@end

NS_ASSUME_NONNULL_END
