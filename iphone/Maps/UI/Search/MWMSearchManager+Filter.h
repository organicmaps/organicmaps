#import "MWMSearchManager.h"
#import "MWMTypes.h"

@interface MWMSearchManager (Filter)

- (void)updateFilter:(MWMVoidBlock)completion;
- (void)clearFilter;

@end
