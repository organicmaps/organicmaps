#import "MWMPlacePageButtonCell.h"
#import "Common.h"
#import "MWMFrameworkListener.h"
#import "MWMPlacePageProtocol.h"
#import "MWMPlacePageViewManager.h"
#import "UIColor+MapsMeColor.h"

@interface MWMPlacePageButtonCell ()<MWMFrameworkStorageObserver>

@property(weak, nonatomic) MWMPlacePageViewManager * manager;
@property(weak, nonatomic) IBOutlet UIButton * titleButton;
@property(nonatomic) MWMPlacePageCellType type;
@property(nonatomic) storage::TCountryId countryId;

@property(weak, nonatomic) id<MWMPlacePageButtonsProtocol> delegate;
@property(nonatomic) place_page::ButtonsRows rowType;
@end

@implementation MWMPlacePageButtonCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  [self.titleButton setTitleColor:[UIColor linkBlueHighlighted] forState:UIControlStateDisabled];
  [self.titleButton setTitleColor:[UIColor linkBlue] forState:UIControlStateNormal];
}

- (void)config:(MWMPlacePageViewManager *)manager forType:(MWMPlacePageCellType)type
{
  self.countryId = GetFramework().GetCountryInfoGetter().GetRegionCountryId(manager.entity.mercator);
  self.manager = manager;
  self.type = type;
  [self refreshButtonEnabledState];
}

- (void)setEnabled:(BOOL)enabled { self.titleButton.enabled = enabled; }
- (BOOL)isEnabled { return self.titleButton.isEnabled; }
- (void)configForRow:(place_page::ButtonsRows)row
        withDelegate:(id<MWMPlacePageButtonsProtocol>)delegate
{
  self.delegate = delegate;
  self.rowType = row;
  NSString * title = nil;
  switch (row)
  {
  case place_page::ButtonsRows::AddPlace:
    title = L(@"placepage_add_place_button");
    break;
  case place_page::ButtonsRows::EditPlace:
    title = L(@"edit_place");
    break;
  case place_page::ButtonsRows::AddBusiness:
    title = L(@"placepage_add_business_button");
    break;
  case place_page::ButtonsRows::HotelDescription:
    title = L(@"details");
    break;
  }

  [self.titleButton setTitle:title forState:UIControlStateNormal];
  [self.titleButton setTitle:title forState:UIControlStateDisabled];
}

- (IBAction)buttonTap
{
  if (IPAD)
  {
    auto m = self.manager;
    switch (self.type)
    {
    case MWMPlacePageCellTypeEditButton: [m editPlace]; break;
    case MWMPlacePageCellTypeAddBusinessButton: [m addBusiness]; break;
    case MWMPlacePageCellTypeAddPlaceButton: [m addPlace]; break;
    case MWMPlacePageCellTypeBookingMore: [m book:YES]; break;
    default: NSAssert(false, @"Incorrect cell type!"); break;
    }
    return;
  }

  using namespace place_page;
  auto d = self.delegate;
  switch (self.rowType)
  {
  case ButtonsRows::AddPlace: [d addPlace]; break;
  case ButtonsRows::EditPlace: [d editPlace]; break;
  case ButtonsRows::AddBusiness: [d addBusiness]; break;
  case ButtonsRows::HotelDescription: [d book:YES]; break;
  }
}

- (void)refreshButtonEnabledState
{
  if (self.countryId == kInvalidCountryId)
  {
    self.titleButton.enabled = YES;
    return;
  }
  NodeStatuses nodeStatuses;
  GetFramework().GetStorage().GetNodeStatuses(self.countryId, nodeStatuses);
  auto const & status = nodeStatuses.m_status;
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
  NSString * title;
  switch (type)
  {
  case MWMPlacePageCellTypeAddBusinessButton:
    title = L(@"placepage_add_business_button");
    [MWMFrameworkListener addObserver:self];
    break;
  case MWMPlacePageCellTypeEditButton:
    title = L(@"edit_place");
    [MWMFrameworkListener addObserver:self];
    break;
  case MWMPlacePageCellTypeAddPlaceButton:
    title = L(@"placepage_add_place_button");
    [MWMFrameworkListener addObserver:self];
    break;
  case MWMPlacePageCellTypeBookingMore:
    title = L(@"details");
    break;
  default: NSAssert(false, @"Invalid place page cell type!"); break;
  }

  [self.titleButton setTitle:title forState:UIControlStateNormal];
  [self.titleButton setTitle:title forState:UIControlStateDisabled];
}

@end
