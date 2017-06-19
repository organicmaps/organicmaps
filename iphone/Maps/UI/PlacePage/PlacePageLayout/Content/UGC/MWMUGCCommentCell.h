#import "MWMTableViewCell.h"

namespace ugc
{
struct Review;
}

@interface MWMUGCCommentCell : MWMTableViewCell

- (void)configWithReview:(ugc::Review const &)review;

@end
