#import "MWMPlacePageButtonCell.h"
#import "MWMFrameworkListener.h"
#import "MWMPlacePageViewManager.h"
#import "UIColor+MapsMeColor.h"

@interface MWMPlacePageButtonCell ()<MWMFrameworkStorageObserver>

@property(weak, nonatomic) MWMPlacePageViewManager * manager;
@property(weak, nonatomic) IBOutlet UIButton * titleButton;
@property(nonatomic) MWMPlacePageCellType type;
@property(nonatomic) storage::TCountryId countryId;

@end

@implementation MWMPlacePageButtonCell

- (void)config:(MWMPlacePageViewManager *)manager forType:(MWMPlacePageCellType)type
{
  self.countryId = GetFramework().CountryInfoGetter().GetRegionCountryId(manager.entity.mercator);
  self.manager = manager;
  self.type = type;
  [self refreshButtonEnabledState];
}

- (IBAction)buttonTap
{
  switch (self.type)
  {
  case MWMPlacePageCellTypeEditButton: [self.manager editPlace]; break;
  case MWMPlacePageCellTypeAddBusinessButton: [self.manager addBusiness]; break;
  case MWMPlacePageCellTypeAddPlaceButton: [self.manager addPlace]; break;
  case MWMPlacePageCellTypeBookingMore: [self.manager book:YES]; break;
  default: NSAssert(false, @"Incorrect cell type!"); break;
  }
}

- (void)refreshButtonEnabledState
{
  NodeAttrs nodeAttrs;
  GetFramework().Storage().GetNodeAttrs(self.countryId, nodeAttrs);
  auto const status = nodeAttrs.m_status;
  self.titleButton.enabled = status == NodeStatus::OnDisk || status == NodeStatus::OnDiskOutOfDate;
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  if (self.countryId != countryId)
    return;
  [self refreshButtonEnabledState];
}

#pragma mark - Properties

- (void)setType:(MWMPlacePageCellType)type
{
  _type = type;
  switch (type)
  {
  case MWMPlacePageCellTypeAddBusinessButton:
    [self.titleButton setTitle:L(@"placepage_add_business_button") forState:UIControlStateNormal];
    [MWMFrameworkListener addObserver:self];
    break;
  case MWMPlacePageCellTypeEditButton:
    [self.titleButton setTitle:L(@"edit_place") forState:UIControlStateNormal];
    [MWMFrameworkListener addObserver:self];
    break;
  case MWMPlacePageCellTypeAddPlaceButton:
    [self.titleButton setTitle:L(@"placepage_add_place_button") forState:UIControlStateNormal];
    [MWMFrameworkListener addObserver:self];
    break;
  case MWMPlacePageCellTypeBookingMore:
    [self.titleButton setTitle:L(@"details") forState:UIControlStateNormal];
    break;
  default: NSAssert(false, @"Invalid place page cell type!"); break;
  }
}

@end
