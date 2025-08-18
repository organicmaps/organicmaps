#import "MWMOpeningHoursAllDayTableViewCell.h"

@interface MWMOpeningHoursAllDayTableViewCell ()

@property(weak, nonatomic) IBOutlet UISwitch * switcher;
@property(weak, nonatomic) IBOutlet UILabel * label;

@end

@implementation MWMOpeningHoursAllDayTableViewCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  [self setupLabelColor];
}

- (void)setupLabelColor
{
  self.label.textColor = self.switcher.on ? [UIColor blackPrimaryText] : [UIColor blackHintText];
}

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
  [self setupLabelColor];
}

@end
