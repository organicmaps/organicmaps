#import "MWMPlacePage.h"
#import "MWMPlacePageButtonCell.h"
#import "Statistics.h"

#import "UIColor+MapsMeColor.h"

@interface MWMPlacePageButtonCell ()

@property (weak, nonatomic) MWMPlacePage * placePage;
@property (weak, nonatomic) IBOutlet UIButton * titleButton;
@property (nonatomic) MWMPlacePageCellType type;

@end

@implementation MWMPlacePageButtonCell

- (void)config:(MWMPlacePage *)placePage forType:(MWMPlacePageCellType)type
{
  self.placePage = placePage;
  switch (type)
  {
  case MWMPlacePageCellTypeAddBusinessButton:
    [self.titleButton setTitleColor:[UIColor linkBlue] forState:UIControlStateNormal];
    [self.titleButton setTitle:L(@"placepage_add_business_button") forState:UIControlStateNormal];
    break;
  case MWMPlacePageCellTypeReportButton:
    [self.titleButton setTitleColor:[UIColor red] forState:UIControlStateNormal];
    [self.titleButton setTitle:L(@"placepage_report_problem_button") forState:UIControlStateNormal];
    break;
  case MWMPlacePageCellTypeEditButton:
    [self.titleButton setTitleColor:[UIColor linkBlue] forState:UIControlStateNormal];
    [self.titleButton setTitle:L(@"edit_place") forState:UIControlStateNormal];
    break;
  default:
    NSAssert(false, @"Invalid place page cell type!");
    break;
  }
  self.type = type;
}

- (IBAction)buttonTap
{
  NSString * key = nil;
  switch (self.type)
  {
  case MWMPlacePageCellTypeEditButton:
    key = kStatEdit;
    [self.placePage editPlace];
    break;
  case MWMPlacePageCellTypeAddBusinessButton:
    key = kStatAddPlace;
    [self.placePage addBusiness];
    break;
  case MWMPlacePageCellTypeReportButton:
    key = kStatReport;
    [self.placePage reportProblem];
    break;
  default:
    NSAssert(false, @"Incorrect cell type!");
    break;
  }
  [Statistics logEvent:kStatEventName(kStatPlacePage, key)];
}

@end
