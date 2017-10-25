#import "MWMPPReviewCell.h"

#include "partners_api/booking_api.hpp"

@interface MWMPPReviewCell ()

@property(weak, nonatomic) IBOutlet UILabel * name;
@property(weak, nonatomic) IBOutlet UILabel * rating;
@property(weak, nonatomic) IBOutlet UILabel * date;
@property(weak, nonatomic) IBOutlet UILabel * positiveReview;
@property(weak, nonatomic) IBOutlet UILabel * negativeReview;
@property(weak, nonatomic) IBOutlet UIImageView * positiveIcon;
@property(weak, nonatomic) IBOutlet UIImageView * negativeIcon;

@property(nonatomic) IBOutletCollection(NSLayoutConstraint) NSArray<NSLayoutConstraint *> * inactiveWhenNoPositiveReviews;
@property(nonatomic) IBOutletCollection(NSLayoutConstraint) NSArray<NSLayoutConstraint *> * inactiveWhenNoNegativeReviews;
@property(nonatomic) IBOutletCollection(NSLayoutConstraint) NSArray<NSLayoutConstraint *> * activateWhenNoPositiveReviews;
@property(nonatomic) IBOutletCollection(NSLayoutConstraint) NSArray<NSLayoutConstraint *> * activateWhenNoNegativeReviews;

@property(nonatomic) IBOutlet NSLayoutConstraint * activeWhenReviewsAreEmpty;

@end

@implementation MWMPPReviewCell

- (void)configWithReview:(booking::HotelReview const &)review
{
  self.name.text = @(review.m_author.c_str());
  self.rating.text = @(review.m_score).stringValue;

  auto formatter = [[NSDateFormatter alloc] init];
  formatter.timeStyle = NSDateFormatterNoStyle;
  formatter.dateStyle = NSDateFormatterLongStyle;

  self.date.text = [formatter stringFromDate:
                    [NSDate dateWithTimeIntervalSince1970:std::chrono::duration_cast<std::chrono::seconds>(review.m_date.time_since_epoch()).count()]];

  self.positiveReview.text = @(review.m_pros.c_str());
  self.negativeReview.text = @(review.m_cons.c_str());

  if (!self.positiveReview.text.length)
  {
    for (NSLayoutConstraint * c in self.inactiveWhenNoPositiveReviews)
      c.active = NO;

    self.positiveIcon.hidden = self.positiveReview.hidden = YES;

    for (NSLayoutConstraint * c in self.activateWhenNoPositiveReviews)
      c.priority = UILayoutPriorityDefaultHigh;
  }

  if (!self.negativeReview.text.length)
  {
    for (NSLayoutConstraint * c in self.inactiveWhenNoNegativeReviews)
      c.active = NO;

    self.negativeIcon.hidden = self.negativeReview.hidden = YES;

    for (NSLayoutConstraint * c in self.activateWhenNoNegativeReviews)
      c.priority = UILayoutPriorityDefaultHigh;
  }

  if (self.negativeIcon.hidden && self.positiveIcon.hidden)
    self.activeWhenReviewsAreEmpty.priority = UILayoutPriorityDefaultHigh;

  [self setNeedsLayout];
}

@end
