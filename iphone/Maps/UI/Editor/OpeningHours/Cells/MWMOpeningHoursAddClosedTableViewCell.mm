#import "MWMOpeningHoursAddClosedTableViewCell.h"

@interface MWMOpeningHoursAddClosedTableViewCell ()

@property(weak, nonatomic) IBOutlet UIButton * addClosedButton;

@end

@implementation MWMOpeningHoursAddClosedTableViewCell

- (void)refresh
{
  [super refresh];
  self.addClosedButton.enabled = self.section.canAddClosedTime;
}

#pragma mark - Actions

- (IBAction)addClosedTap
{
  if (self.isVisible)
    [self.section addClosedTime];
}

@end
