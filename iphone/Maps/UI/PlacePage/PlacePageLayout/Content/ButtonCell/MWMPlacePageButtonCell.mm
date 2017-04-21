#import "MWMPlacePageButtonCell.h"
#import "MWMCommon.h"
#import "MWMPlacePageProtocol.h"
#import "SwiftBridge.h"

@interface MWMPlacePageButtonCell ()

@property(weak, nonatomic) IBOutlet MWMBorderedButton * titleButton;

@property(weak, nonatomic) id<MWMPlacePageButtonsProtocol> delegate;
@property(nonatomic) place_page::ButtonsRows rowType;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * buttonTop;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * buttonTrailing;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * buttonBottom;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * buttonLeading;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * buttonHeight;

@property(nonatomic) BOOL isInsetButton;

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
  BOOL isInsetButton = NO;
  switch (row)
  {
  case ButtonsRows::AddPlace:
    title = L(@"placepage_add_place_button");
    isInsetButton = YES;
    break;
  case ButtonsRows::EditPlace:
    title = L(@"edit_place");
    isInsetButton = YES;
    break;
  case ButtonsRows::AddBusiness:
    title = L(@"placepage_add_business_button");
    isInsetButton = YES;
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
  case ButtonsRows::LocalAdsCandidate:
    title = L(@"create_campaign_button");
    isInsetButton = YES;
    break;
  case ButtonsRows::LocalAdsCustomer:
    title = L(@"view_campaign_button");
    isInsetButton = YES;
    break;
  }

  self.isInsetButton = isInsetButton;
  [self.titleButton setTitle:title forState:UIControlStateNormal];
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
  case ButtonsRows::LocalAdsCandidate:
  case ButtonsRows::LocalAdsCustomer: [d openLocalAdsURL]; break;
  }
}

#pragma mark - Properties

- (void)setIsInsetButton:(BOOL)isInsetButton
{
  _isInsetButton = isInsetButton;
  auto titleButton = self.titleButton;
  auto btnLayer = titleButton.layer;
  if (isInsetButton)
  {
    self.backgroundColor = [UIColor clearColor];
    self.buttonTop.constant = 8;
    self.buttonTrailing.constant = 16;
    self.buttonBottom.constant = 8;
    self.buttonLeading.constant = 16;
    self.buttonHeight.constant = 36;

    [titleButton setBorderColor:[UIColor linkBlue]];
    [titleButton setBorderHighlightedColor:[UIColor linkBlueHighlighted]];
    btnLayer.borderWidth = 1;
    btnLayer.borderColor = [UIColor linkBlue].CGColor;
    btnLayer.cornerRadius = 4;

    self.isSeparatorHidden = YES;
  }
  else
  {
    self.backgroundColor = [UIColor white];
    self.buttonTop.constant = 0;
    self.buttonTrailing.constant = 0;
    self.buttonBottom.constant = 0;
    self.buttonLeading.constant = 0;
    self.buttonHeight.constant = 44;

    btnLayer.borderWidth = 0;
  }
}

@end
