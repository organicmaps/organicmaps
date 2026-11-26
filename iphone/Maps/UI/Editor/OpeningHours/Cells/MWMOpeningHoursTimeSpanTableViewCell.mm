#import "MWMOpeningHoursTimeSpanTableViewCell.h"

@interface MWMOpeningHoursTimeSpanTableViewCell ()

@property(weak, nonatomic) IBOutlet UILabel * openTimeLabel;
@property(weak, nonatomic) IBOutlet UILabel * closeTimeLabel;

@end

@implementation MWMOpeningHoursTimeSpanTableViewCell

- (void)refresh
{
  [super refresh];
  NSCalendar * calendar = NSCalendar.currentCalendar;

  MWMOpeningHoursSection * section = self.section;
  NSUInteger const row = self.row;
  NSDate * openDate = [calendar dateFromComponents:[section timeForRow:row isStart:YES]];
  NSDate * closeDate = [calendar dateFromComponents:[section timeForRow:row isStart:NO]];

  NSDateFormatterStyle timeStyle = NSDateFormatterShortStyle;
  NSDateFormatterStyle dateStyle = NSDateFormatterNoStyle;
  self.openTimeLabel.text = [DateTimeFormatter dateStringFrom:openDate dateStyle:dateStyle timeStyle:timeStyle];
  self.closeTimeLabel.text = [DateTimeFormatter dateStringFrom:closeDate dateStyle:dateStyle timeStyle:timeStyle];

  BOOL const isRowSelected = [section isRowSelected:row];
  self.isAccessibilityElement = YES;
  self.accessibilityLabel = [NSString
      stringWithFormat:L(@"editor_time_span_accessibility"), self.openTimeLabel.text, self.closeTimeLabel.text];
  self.accessibilityTraits = UIAccessibilityTraitButton | (isRowSelected ? UIAccessibilityTraitSelected : 0);

  UIColor * clr = isRowSelected ? [UIColor linkBlue] : [UIColor blackSecondaryText];
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

- (BOOL)accessibilityActivate
{
  if (!self.isVisible)
    return NO;
  [self expandTap];
  return YES;
}

@end
