#import "MWMOpeningHoursAddScheduleTableViewCell.h"
#import "MWMOpeningHoursEditorViewController.h"

@interface MWMOpeningHoursAddScheduleTableViewCell ()

@property (weak, nonatomic) IBOutlet UIButton * addScheduleButton;

@end

@implementation MWMOpeningHoursAddScheduleTableViewCell

+ (CGFloat)height
{
  return 84.0;
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
  NSString * unhandledDays = [model.unhandledDays componentsJoinedByString:@", "];
  NSString * title = [NSString stringWithFormat:@"%@ %@", L(@"add_schedule_for"), unhandledDays];
  [self.addScheduleButton setTitle:title forState:UIControlStateNormal];
}

@end
