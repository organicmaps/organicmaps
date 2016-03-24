#import "MWMPlacePage.h"
#import "MWMPlacePageButtonCell.h"
#import "Statistics.h"

#import "UIColor+MapsMeColor.h"

@interface MWMPlacePageButtonCell ()

@property (weak, nonatomic) MWMPlacePage * placePage;
@property (weak, nonatomic) IBOutlet UIButton * titleButton;
@property (nonatomic) BOOL isReport;

@end

@implementation MWMPlacePageButtonCell

- (void)config:(MWMPlacePage *)placePage isReport:(BOOL)isReport
{
  self.placePage = placePage;
  self.isReport = isReport;
  [self.titleButton setTitleColor:isReport ? [UIColor red] : [UIColor linkBlue] forState:UIControlStateNormal];
  [self.titleButton setTitle:isReport ? L(@"placepage_report_problem_button") : L(@"edit_place") forState:UIControlStateNormal];
}

- (IBAction)buttonTap
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, self.isReport ? kStatReport : kStatEdit)];
  if (self.isReport)
    [self.placePage reportProblem];
  else
    [self.placePage editPlace];
}

@end
