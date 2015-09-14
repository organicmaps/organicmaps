#include "search/result.hpp"

@interface MWMSearchCell : UITableViewCell

- (void)config:(search::Result &)result;

@property (nonatomic) CGFloat preferredMaxLayoutWidth;

@end
