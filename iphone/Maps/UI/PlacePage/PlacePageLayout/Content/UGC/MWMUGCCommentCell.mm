#import "MWMUGCCommentCell.h"
#import "UIImageView+Coloring.h"

#include "ugc/types.hpp"

namespace
{
NSString * formattedDateFrom(ugc::Time time)
{
  NSDateComponentsFormatter * formatter = [[NSDateComponentsFormatter alloc] init];
  formatter.unitsStyle = NSDateComponentsFormatterUnitsStyleFull;

  using namespace std::chrono;
  NSTimeInterval const t = duration_cast<seconds>(time.time_since_epoch()).count();

  NSDate * date = [NSDate dateWithTimeIntervalSince1970:t];
  NSCalendar * calendar = NSCalendar.currentCalendar;
  auto const mask = (NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitWeekOfMonth |
                NSCalendarUnitDay | NSCalendarUnitHour | NSCalendarUnitMinute);
  NSDateComponents * components = [calendar components:mask fromDate:date];

  if (components.year > 0) {
    formatter.allowedUnits = NSCalendarUnitYear;
  } else if (components.month > 0) {
    formatter.allowedUnits = NSCalendarUnitMonth;
  } else if (components.weekOfMonth > 0) {
    formatter.allowedUnits = NSCalendarUnitWeekOfMonth;
  } else if (components.day > 0) {
    formatter.allowedUnits = NSCalendarUnitDay;
  } else if (components.hour > 0) {
    formatter.allowedUnits = NSCalendarUnitHour;
  } else if (components.minute > 0) {
    formatter.allowedUnits = NSCalendarUnitMinute;
  } else {
    formatter.allowedUnits = NSCalendarUnitSecond;
  }

  return [formatter stringFromDateComponents:components];
}
}

@interface MWMUGCCommentCell ()

@property(weak, nonatomic) IBOutlet UILabel * author;
@property(weak, nonatomic) IBOutlet UILabel * date;
@property(weak, nonatomic) IBOutlet UILabel * comment;
@property(weak, nonatomic) IBOutlet UILabel * positiveCount;
@property(weak, nonatomic) IBOutlet UILabel * negativeCount;
@property(weak, nonatomic) IBOutlet UIImageView * thumbUp;
@property(weak, nonatomic) IBOutlet UIImageView * thumbDown;
@property(weak, nonatomic) IBOutlet UIButton * thumbUpButton;
@property(weak, nonatomic) IBOutlet UIButton * thumbDownButton;
@property(weak, nonatomic) IBOutlet UIButton * showOriginal;

@end

@implementation MWMUGCCommentCell

- (void)configWithReview:(ugc::Review const &)review
{
  self.author.text = @(review.m_author.m_name.c_str());
  self.comment.text = @(review.m_text.m_text.c_str());
  self.date.text = formattedDateFrom(review.m_time);
}

#pragma mark - Actions

- (IBAction)thumbUpTap
{
  self.thumbUp.mwm_coloring = self.thumbUpButton.isSelected ? MWMImageColoringBlack : MWMImageColoringGray;
  // TODO: Increment positive count
  if (self.thumbDownButton.selected)
  {
    self.thumbDown.mwm_coloring = MWMImageColoringGray;
    self.thumbDownButton.selected = NO;
    // Decrement negative count
  }
}

- (IBAction)thumbDownTap
{
  self.thumbDown.mwm_coloring = self.thumbDownButton.isSelected ? MWMImageColoringBlack : MWMImageColoringGray;
  // TODO: Increment negative count
  if (self.thumbUpButton.selected)
  {
    self.thumbUp.mwm_coloring = MWMImageColoringGray;
    self.thumbUpButton.selected = NO;
    // Decrement positive count
  }
}

- (IBAction)showOriginalTap
{
  // TODO: Show original comment. Will be implemented soon.
}

@end
