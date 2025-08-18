#import "MWMOpeningHoursDeleteScheduleTableViewCell.h"

@implementation MWMOpeningHoursDeleteScheduleTableViewCell

+ (CGFloat)heightForWidth:(CGFloat)width
{
  return 44.0;
}

- (IBAction)deleteScheduleTap
{
  [self.section deleteSchedule];
}

@end
