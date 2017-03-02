#import "MWMPlacePageButtonCell.h"
#import "MWMCommon.h"
#import "MWMPlacePageProtocol.h"

@interface MWMPlacePageButtonCell ()

@property(weak, nonatomic) IBOutlet UIButton * titleButton;

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

- (void)setEnabled:(BOOL)enabled { self.titleButton.enabled = enabled; }
- (BOOL)isEnabled { return self.titleButton.isEnabled; }
- (void)configForRow:(place_page::ButtonsRows)row
        withDelegate:(id<MWMPlacePageButtonsProtocol>)delegate
{
  using place_page::ButtonsRows;

  self.delegate = delegate;
  self.rowType = row;
  NSString * title = nil;
  switch (row)
  {
  case ButtonsRows::AddPlace:
    title = L(@"placepage_add_place_button");
    break;
  case ButtonsRows::EditPlace:
    title = L(@"edit_place");
    break;
  case ButtonsRows::AddBusiness:
    title = L(@"placepage_add_business_button");
    break;
  case ButtonsRows::HotelDescription:
    title = L(@"details_on_bookingcom");
    break;
  case ButtonsRows::BookingShowMoreFacilities:
    title = L(@"booking_show_more");
    break;
  case ButtonsRows::BookingShowMoreReviews:
    title = L(@"reviews_on_bookingcom");
    break;
  case ButtonsRows::BookingShowMoreOnSite:
    title = L(@"more_on_bookingcom");
    break;
  }

  [self.titleButton setTitle:title forState:UIControlStateNormal];
  [self.titleButton setTitle:title forState:UIControlStateDisabled];
}

- (IBAction)buttonTap
{
  using place_page::ButtonsRows;
  
  auto d = self.delegate;
  switch (self.rowType)
  {
  case ButtonsRows::AddPlace: [d addPlace]; break;
  case ButtonsRows::EditPlace: [d editPlace]; break;
  case ButtonsRows::AddBusiness: [d addBusiness]; break;
  case ButtonsRows::BookingShowMoreOnSite:
  case ButtonsRows::HotelDescription: [d book:YES]; break;
  case ButtonsRows::BookingShowMoreFacilities: [d showAllFacilities]; break;
  case ButtonsRows::BookingShowMoreReviews: [d showAllReviews]; break;
  }
}

@end
