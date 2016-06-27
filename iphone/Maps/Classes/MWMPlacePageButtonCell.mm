#import "MWMPlacePageButtonCell.h"
#import "MWMPlacePageViewManager.h"
#import "UIColor+MapsMeColor.h"

@interface MWMPlacePageButtonCell ()

@property (weak, nonatomic) MWMPlacePageViewManager * manager;
@property (weak, nonatomic) IBOutlet UIButton * titleButton;
@property (nonatomic) MWMPlacePageCellType type;

@end

@implementation MWMPlacePageButtonCell

- (void)config:(MWMPlacePageViewManager *)manager forType:(MWMPlacePageCellType)type
{
  self.manager = manager;
  switch (type)
  {
  case MWMPlacePageCellTypeAddBusinessButton:
    [self.titleButton setTitle:L(@"placepage_add_business_button") forState:UIControlStateNormal];
    break;
  case MWMPlacePageCellTypeEditButton:
    [self.titleButton setTitle:L(@"edit_place") forState:UIControlStateNormal];
    break;
  case MWMPlacePageCellTypeAddPlaceButton:
    [self.titleButton setTitle:L(@"placepage_add_place_button") forState:UIControlStateNormal];
    break;
  case MWMPlacePageCellTypeBookingMore:
    [self.titleButton setTitle:L(@"details") forState:UIControlStateNormal];
    break;
  default:
    NSAssert(false, @"Invalid place page cell type!");
    break;
  }
  self.type = type;
}

- (IBAction)buttonTap
{
  switch (self.type)
  {
  case MWMPlacePageCellTypeEditButton:
    [self.manager editPlace];
    break;
  case MWMPlacePageCellTypeAddBusinessButton:
    [self.manager addBusiness];
    break;
  case MWMPlacePageCellTypeAddPlaceButton:
    [self.manager addPlace];
    break;
  case MWMPlacePageCellTypeBookingMore:
    [self.manager book:YES];
    break;
  default:
    NSAssert(false, @"Incorrect cell type!");
    break;
  }
}

@end
