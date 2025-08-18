#import "MWMOpeningHoursAddScheduleTableViewCell.h"
#import <CoreApi/MWMOpeningHoursCommon.h>

@interface MWMOpeningHoursAddScheduleTableViewCell ()

@property(weak, nonatomic) IBOutlet UIButton * addScheduleButton;

@end

@implementation MWMOpeningHoursAddScheduleTableViewCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.backgroundColor = UIColor.clearColor;
}

+ (CGFloat)height
{
  return 84.0;
}

- (void)refresh
{
  NSString * title =
      [NSString stringWithFormat:@"%@ %@", L(@"editor_time_add"), stringFromOpeningDays([self.model unhandledDays])];
  [self.addScheduleButton setTitle:title forState:UIControlStateNormal];
}

#pragma mark - Actions

- (IBAction)addScheduleTap
{
  [self.model addSchedule];
}

#pragma mark - Properties

- (void)setModel:(MWMOpeningHoursModel *)model
{
  _model = model;
  [self refresh];
}

@end
