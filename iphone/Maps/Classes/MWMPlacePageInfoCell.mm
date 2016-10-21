#import "MWMPlacePageInfoCell.h"
#import "Common.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"
#import "UIImageView+Coloring.h"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

@interface MWMPlacePageInfoCell ()<UITextViewDelegate>

@property(weak, nonatomic, readwrite) IBOutlet UIImageView * icon;
@property(weak, nonatomic, readwrite) IBOutlet id textContainer;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * textContainerHeight;

@property(weak, nonatomic) IBOutlet UIButton * upperButton;
@property(weak, nonatomic) IBOutlet UIImageView * toggleImage;

@property(nonatomic) MWMPlacePageCellType type NS_DEPRECATED_IOS(7_0, 8_0, "Use rowType instead");
@property(nonatomic) place_page::MetainfoRows rowType NS_AVAILABLE_IOS(8_0);
@property(weak, nonatomic) MWMPlacePageData * data NS_AVAILABLE_IOS(8_0);

@end

@implementation MWMPlacePageInfoCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  if ([self.textContainer isKindOfClass:[UITextView class]])
  {
    UITextView * textView = (UITextView *)self.textContainer;
    textView.textContainerInset = {.left = -5, .top = 12};
    textView.keyboardAppearance =
        [UIColor isNightMode] ? UIKeyboardAppearanceDark : UIKeyboardAppearanceDefault;
  }
}

- (void)configWithRow:(place_page::MetainfoRows)row data:(MWMPlacePageData *)data;
{
  self.rowType = row;
  self.data = data;
  NSString * name;
  switch (row)
  {
  case place_page::MetainfoRows::Address:
    self.toggleImage.hidden = YES;
    name = @"address";
    break;
  case place_page::MetainfoRows::Phone:
    self.toggleImage.hidden = YES;
    name = @"phone_number";
    break;
  case place_page::MetainfoRows::Website:
    self.toggleImage.hidden = YES;
    name = @"website";
    break;
  case place_page::MetainfoRows::Email:
    self.toggleImage.hidden = YES;
    name = @"email";
    break;
  case place_page::MetainfoRows::Cuisine:
    self.toggleImage.hidden = YES;
    name = @"cuisine";
    break;
  case place_page::MetainfoRows::Operator:
    self.toggleImage.hidden = YES;
    name = @"operator";
    break;
  case place_page::MetainfoRows::Internet:
    self.toggleImage.hidden = YES;
    name = @"wifi";
    break;
  case place_page::MetainfoRows::Coordinate:
    self.toggleImage.hidden = NO;
    name = @"coordinate";
    break;
  case place_page::MetainfoRows::OpeningHours: NSAssert(false, @"Incorrect cell type!"); break;
  }
  [self configWithIconName:name data:[data stringForRow:row]];
}

- (void)configureWithType:(MWMPlacePageCellType)type info:(NSString *)info;
{
  self.type = type;
  NSString * typeName;
  switch (type)
  {
  case MWMPlacePageCellTypeURL:
  case MWMPlacePageCellTypeWebsite:
    self.toggleImage.hidden = YES;
    typeName = @"website";
    break;
  case MWMPlacePageCellTypeEmail:
    self.toggleImage.hidden = YES;
    typeName = @"email";
    break;
  case MWMPlacePageCellTypePhoneNumber:
    self.toggleImage.hidden = YES;
    typeName = @"phone_number";
    break;
  case MWMPlacePageCellTypeCoordinate:
    self.toggleImage.hidden = NO;
    typeName = @"coordinate";
    break;
  case MWMPlacePageCellTypePostcode:
    self.toggleImage.hidden = YES;
    typeName = @"postcode";
    break;
  case MWMPlacePageCellTypeWiFi:
    self.toggleImage.hidden = YES;
    typeName = @"wifi";
    break;
  default: NSAssert(false, @"Incorrect type!"); break;
  }

  [self configWithIconName:typeName data:info];
}

- (void)configWithIconName:(NSString *)name data:(NSString *)data
{
  UIImage * image =
      [UIImage imageNamed:[NSString stringWithFormat:@"%@%@", @"ic_placepage_", name]];
  self.icon.image = image;
  self.icon.mwm_coloring = [self.textContainer isKindOfClass:[UITextView class]]
                               ? MWMImageColoringBlue
                               : MWMImageColoringBlack;
  [self changeText:data];
  UILongPressGestureRecognizer * longTap =
      [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(longTap:)];
  longTap.minimumPressDuration = 0.3;
  [self.upperButton addGestureRecognizer:longTap];
}

- (void)changeText:(NSString *)text
{
  if ([self.textContainer isKindOfClass:[UITextView class]])
  {
    UITextView * tv = (UITextView *)self.textContainer;
    [tv setAttributedText:[[NSAttributedString alloc]
                              initWithString:text
                                  attributes:@{NSFontAttributeName : [UIFont regular16]}]];
    [tv sizeToIntegralFit];
    CGFloat const minTextContainerHeight = 42.0;
    CGFloat const bottomOffset = 8.0;
    self.textContainerHeight.constant =
        MAX(ceil(tv.contentSize.height) + bottomOffset, minTextContainerHeight);
  }
  else
  {
    UILabel * lb = (UILabel *)self.textContainer;
    [lb setText:text];
    [lb sizeToIntegralFit];
    CGFloat const trailingOffset = self.width - lb.maxX;
    lb.font = trailingOffset < 32 ? [UIFont regular15] : [UIFont regular16];
  }
}

- (BOOL)textView:(UITextView *)textView
    shouldInteractWithURL:(NSURL *)URL
                  inRange:(NSRange)characterRange
{
  NSString * scheme = URL.scheme;
  if ([scheme isEqualToString:@"http"] || [scheme isEqualToString:@"https"])
  {
    [[MapViewController controller] openUrl:URL];
    return NO;
  }
  return YES;
}

- (IBAction)cellTap
{
  if (IPAD)
  {
    switch (self.type)
    {
    case MWMPlacePageCellTypeURL:
    case MWMPlacePageCellTypeWebsite:
      [Statistics logEvent:kStatEventName(kStatPlacePage, kStatOpenSite)];
      break;
    case MWMPlacePageCellTypeEmail:
      [Statistics logEvent:kStatEventName(kStatPlacePage, kStatSendEmail)];
      break;
    case MWMPlacePageCellTypePhoneNumber:
      [Statistics logEvent:kStatEventName(kStatPlacePage, kStatCallPhoneNumber)];
      break;
    case MWMPlacePageCellTypeCoordinate:
      [Statistics logEvent:kStatEventName(kStatPlacePage, kStatToggleCoordinates)];
      [self.currentEntity toggleCoordinateSystem];
      [self changeText:[self.currentEntity getCellValue:MWMPlacePageCellTypeCoordinate]];
      break;
    default: break;
    }
    return;
  }

  switch (self.rowType)
  {
  case place_page::MetainfoRows::Phone:
    [Statistics logEvent:kStatEventName(kStatPlacePage, kStatCallPhoneNumber)];
    break;
  case place_page::MetainfoRows::Website:
    [Statistics logEvent:kStatEventName(kStatPlacePage, kStatOpenSite)];
    break;
  case place_page::MetainfoRows::Email:
    [Statistics logEvent:kStatEventName(kStatPlacePage, kStatSendEmail)];
    break;
  case place_page::MetainfoRows::Coordinate:
    [Statistics logEvent:kStatEventName(kStatPlacePage, kStatToggleCoordinates)];
    [MWMPlacePageData toggleCoordinateSystem];
    [self changeText:[self.data stringForRow:self.rowType]];
    break;

  case place_page::MetainfoRows::Cuisine:
  case place_page::MetainfoRows::Operator:
  case place_page::MetainfoRows::OpeningHours:
  case place_page::MetainfoRows::Address:
  case place_page::MetainfoRows::Internet: break;
  }
}

- (void)longTap:(UILongPressGestureRecognizer *)sender
{
  UIMenuController * menuController = [UIMenuController sharedMenuController];
  if (menuController.isMenuVisible)
    return;
  CGPoint const tapPoint = [sender locationInView:sender.view.superview];
  UIView * targetView =
      [self.textContainer isKindOfClass:[UITextView class]] ? sender.view : self.textContainer;
  [menuController setTargetRect:CGRectMake(tapPoint.x, targetView.minY, 0., 0.)
                         inView:sender.view.superview];
  [menuController setMenuVisible:YES animated:YES];
  [targetView becomeFirstResponder];
  [menuController update];
}

@end
