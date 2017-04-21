#import "MWMSearchCell.h"

#include "search/result.hpp"

@interface MWMSearchCommonCell : MWMSearchCell

- (void)config:(search::Result const &)result isLocalAds:(BOOL)isLocalAds;

@end
