#import "MWMOpeningHoursTimeSpanTableViewCell.h"

@interface MWMOpeningHoursTimeSpanTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * openTimeLabel;
@property (weak, nonatomic) IBOutlet UILabel * closeTimeLabel;

@end

@implementation MWMOpeningHoursTimeSpanTableViewCell

+ (CGFloat)heightForWidth:(CGFloat)width
{
  return 64.0;
}

- (void)refresh
{
  [super refresh];
  NSLocale * locale = NSLocale.currentLocale;
  NSCalendar * calendar = NSCalendar.currentCalendar;
  calendar.locale = locale;

  NSDateFormatter * dateFormatter = [[NSDateFormatter alloc] init];
  dateFormatter.locale = locale;
  dateFormatter.timeStyle = NSDateFormatterShortStyle;
  dateFormatter.dateStyle = NSDateFormatterNoStyle;

  MWMOpeningHoursSection * section = self.section;
  NSUInteger const row = self.row;
  NSDate * openDate = [calendar dateFromComponents:[section timeForRow:row isStart:YES]];
  NSDate * closeDate = [calendar dateFromComponents:[section timeForRow:row isStart:NO]];

  self.openTimeLabel.text = [dateFormatter stringFromDate:openDate];
  self.closeTimeLabel.text = [dateFormatter stringFromDate:closeDate];

  UIColor * clr = [section isRowSelected:row] ? [UIColor linkBlue] : [UIColor blackSecondaryText];
  self.openTimeLabel.textColor = clr;
  self.closeTimeLabel.textColor = clr;
}

#pragma mark - Actions

- (IBAction)expandTap
{
  if (self.isVisible)
  {
    MWMOpeningHoursSection * section = self.section;
    NSUInteger const row = self.row;
    section.selectedRow = [section isRowSelected:row] ? nil : @(row);
    [section refresh:NO];
  }
}

@end
