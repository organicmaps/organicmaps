#import "MWMSearchCell.h"

#include "map/everywhere_search_callback.hpp"
#include "search/result.hpp"

@interface MWMSearchCommonCell : MWMSearchCell

- (void)config:(search::Result const &)result
    isAvailable:(BOOL)isAvailable
    productInfo:(search::ProductInfo const &)productInfo;

@end
