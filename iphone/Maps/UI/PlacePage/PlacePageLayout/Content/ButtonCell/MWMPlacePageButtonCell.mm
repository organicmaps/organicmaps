#import "MWMPlacePageButtonCell.h"
#import "MWMPlacePageData.h"
#import "SwiftBridge.h"

@interface MWMPlacePageButtonCell ()

@property(weak, nonatomic) IBOutlet MWMBorderedButton * titleButton;

@property(nonatomic) place_page::ButtonsRows rowType;
@property(copy, nonatomic) MWMVoidBlock action;

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

- (void)configWithTitle:(NSString *)title
                 action:(MWMVoidBlock)action
          isInsetButton:(BOOL)isInsetButton
{
  self.rowType = place_page::ButtonsRows::Other;
  self.action = action;
  self.isInsetButton = isInsetButton;
  [self.titleButton setTitle:title forState:UIControlStateNormal];
}

- (void)configForRow:(place_page::ButtonsRows)row
        withAction:(MWMVoidBlock)action
{
  using place_page::ButtonsRows;

  self.rowType = row;
  self.action = action;
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
    isInsetButton = YES;
    break;
  case ButtonsRows::Other:
    break;
  }

  self.isInsetButton = isInsetButton;
  [self.titleButton setTitle:title forState:UIControlStateNormal];
}

- (IBAction)buttonTap
{
  if (self.action)
    self.action();
}

#pragma mark - Properties

- (void)setIsInsetButton:(BOOL)isInsetButton
{
  _isInsetButton = isInsetButton;
  auto titleButton = self.titleButton;
  auto btnLayer = titleButton.layer;
  if (isInsetButton)
  {
    self.backgroundColor = UIColor.clearColor;
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
