#import "MWMOpeningHoursAllDayTableViewCell.h"

@interface MWMOpeningHoursAllDayTableViewCell ()

@property (weak, nonatomic) IBOutlet UISwitch * switcher;

@end

@implementation MWMOpeningHoursAllDayTableViewCell

+ (CGFloat)heightForWidth:(CGFloat)width
{
  return 44.0;
}

- (void)refresh
{
  [super refresh];
  self.switcher.on = self.section.allDay;
}

#pragma mark - Actions

- (IBAction)onSwitch
{
  self.section.allDay = self.switcher.on;
}

@end
