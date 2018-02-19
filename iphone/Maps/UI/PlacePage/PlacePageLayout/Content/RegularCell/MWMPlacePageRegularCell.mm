#import "MWMPlacePageRegularCell.h"
#import "MWMCommon.h"
#import "MWMLocationManager.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "UIImageView+Coloring.h"

#include "local_ads/event.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

@interface MWMPlacePageRegularCell ()<UITextViewDelegate>

@property(weak, nonatomic, readwrite) IBOutlet UIImageView * icon;
@property(weak, nonatomic, readwrite) IBOutlet id textContainer;

@property(weak, nonatomic) IBOutlet UIButton * upperButton;
@property(weak, nonatomic) IBOutlet UIImageView * toggleImage;

@property(nonatomic) place_page::MetainfoRows rowType;
@property(weak, nonatomic) MWMPlacePageData * data;

@end

@implementation MWMPlacePageRegularCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  if ([self.textContainer isKindOfClass:[UITextView class]])
  {
    UITextView * textView = (UITextView *)self.textContainer;
    textView.textContainerInset = {.left = -5, .top = 12, .bottom = 12};
    textView.keyboardAppearance =
        [UIColor isNightMode] ? UIKeyboardAppearanceDark : UIKeyboardAppearanceDefault;
  }
  [self.icon layoutIfNeeded];
}

- (void)configWithRow:(place_page::MetainfoRows)row data:(MWMPlacePageData *)data;
{
  using place_page::MetainfoRows;
  self.rowType = row;
  self.data = data;
  NSString * name;
  switch (row)
  {
  case MetainfoRows::Address:
    self.toggleImage.hidden = YES;
    name = @"address";
    break;
  case MetainfoRows::Phone:
    self.toggleImage.hidden = YES;
    name = @"phone_number";
    break;
  case MetainfoRows::Website:
    self.toggleImage.hidden = YES;
    name = @"website";
    break;
  case MetainfoRows::Email:
    self.toggleImage.hidden = YES;
    name = @"email";
    break;
  case MetainfoRows::Cuisine:
    self.toggleImage.hidden = YES;
    name = @"cuisine";
    break;
  case MetainfoRows::Operator:
    self.toggleImage.hidden = YES;
    name = @"operator";
    break;
  case MetainfoRows::Internet:
    self.toggleImage.hidden = YES;
    name = @"core_wifi";
    break;
  case MetainfoRows::Coordinate:
    self.toggleImage.hidden = NO;
    name = @"coordinate";
    break;
  case MetainfoRows::ExtendedOpeningHours:
  case MetainfoRows::OpeningHours:
  case MetainfoRows::LocalAdsCandidate:
  case MetainfoRows::LocalAdsCustomer: break;
  }
  [self configWithIconName:name data:[data stringForRow:row]];
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
  }
  else
  {
    UILabel * lb = (UILabel *)self.textContainer;
    [lb setText:text];
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
  using place_page::MetainfoRows;
  auto data = self.data;
  switch (self.rowType)
  {
  case MetainfoRows::Phone:
    [Statistics logEvent:kStatEventName(kStatPlacePage, kStatCallPhoneNumber)];
    [data logLocalAdsEvent:local_ads::EventType::ClickedPhone];
    break;
  case MetainfoRows::Website:
    [Statistics logEvent:kStatEventName(kStatPlacePage, kStatOpenSite)];
    [data logLocalAdsEvent:local_ads::EventType::ClickedWebsite];
    break;
  case MetainfoRows::Email:
    [Statistics logEvent:kStatEventName(kStatPlacePage, kStatSendEmail)];
    break;
  case MetainfoRows::Coordinate:
    [Statistics logEvent:kStatEventName(kStatPlacePage, kStatToggleCoordinates)];
    [MWMPlacePageData toggleCoordinateSystem];
    [self changeText:[data stringForRow:self.rowType]];
    break;
  case MetainfoRows::ExtendedOpeningHours:
  case MetainfoRows::Cuisine:
  case MetainfoRows::Operator:
  case MetainfoRows::OpeningHours:
  case MetainfoRows::Address:
  case MetainfoRows::Internet:
  case MetainfoRows::LocalAdsCustomer: 
  case MetainfoRows::LocalAdsCandidate: break;
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
  UIView * superview = sender.view.superview;
  [menuController setTargetRect:CGRectMake(tapPoint.x, targetView.minY, 0., 0.) inView:superview];
  [menuController setMenuVisible:YES animated:YES];
  [targetView becomeFirstResponder];
  [menuController update];
}

@end

@implementation MWMPlacePageInfoCell
@end

@implementation MWMPlacePageLinkCell
@end
