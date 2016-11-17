#import "MWMPlacePageButtonCell.h"
#import "Common.h"
#import "MWMPlacePageProtocol.h"
#import "UIColor+MapsMeColor.h"

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

@end
