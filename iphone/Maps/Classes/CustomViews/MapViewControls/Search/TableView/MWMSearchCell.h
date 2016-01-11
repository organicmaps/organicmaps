#import "MWMTableViewCell.h"

#include "search/result.hpp"

@interface MWMSearchCell : MWMTableViewCell

- (void)config:(search::Result &)result;

@end
