#import "MWMTableViewCell.h"

namespace booking
{
struct HotelReview;
}

@interface MWMPPReviewCell : MWMTableViewCell

- (void)configWithReview:(booking::HotelReview const &)review;

@end
