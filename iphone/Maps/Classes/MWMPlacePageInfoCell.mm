#import "Common.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageInfoCell.h"
#import "Statistics.h"
#import "UIFont+MapsMeFonts.h"
#import "UIImageView+Coloring.h"

#include "platform/settings.hpp"
#include "platform/measurement_utils.hpp"

extern NSString * const kUserDefaultsLatLonAsDMSKey;

@interface MWMPlacePageInfoCell () <UITextViewDelegate>

@property (weak, nonatomic, readwrite) IBOutlet UIImageView * icon;
@property (weak, nonatomic, readwrite) IBOutlet id textContainer;

@property (weak, nonatomic) IBOutlet UIButton * upperButton;
@property (nonatomic) MWMPlacePageMetadataType type;

@end

@implementation MWMPlacePageInfoCell

- (void)configureWithType:(MWMPlacePageMetadataType)type info:(NSString *)info;
{
  NSString * typeName = nil;
  switch (type)
  {
    case MWMPlacePageMetadataTypeURL:
    case MWMPlacePageMetadataTypeWebsite:
      typeName = @"website";
      break;
    case MWMPlacePageMetadataTypeEmail:
      typeName = @"email";
      break;
    case MWMPlacePageMetadataTypePhoneNumber:
      typeName = @"phone_number";
      break;
    case MWMPlacePageMetadataTypeCoordinate:
      typeName = @"coordinate";
      break;
    case MWMPlacePageMetadataTypePostcode:
      typeName = @"postcode";
      break;
    case MWMPlacePageMetadataTypeOpenHours:
      typeName = @"open_hours";
      break;
    case MWMPlacePageMetadataTypeWiFi:
      typeName = @"wifi";
      break;
    case MWMPlacePageMetadataTypeBookmark:
      NSAssert(false, @"Incorrect type!");
      break;
  }
  
  UIImage * image = [UIImage imageNamed:[NSString stringWithFormat:@"%@%@", @"ic_placepage_", typeName]];
  self.type = type;
  self.icon.image = image;

  if ([self.textContainer isKindOfClass:[UITextView class]])
  {
    [self.textContainer setAttributedText:[[NSAttributedString alloc] initWithString:info attributes:@{NSFontAttributeName : [UIFont light16]}]];
    self.icon.mwm_coloring = MWMImageColoringBlue;
  }
  else
  {
    [self.textContainer setText:info];
    self.icon.mwm_coloring = MWMImageColoringBlack;
  }

  UILongPressGestureRecognizer * longTap = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(longTap:)];
  longTap.minimumPressDuration = 0.3;
  [self.upperButton addGestureRecognizer:longTap];
}

- (BOOL)textView:(UITextView *)textView shouldInteractWithURL:(NSURL *)URL inRange:(NSRange)characterRange
{
  return YES;
}

- (void)layoutSubviews
{
  CGFloat const leftOffset = 16.;
  CGFloat const topOffset = 8.;
  CGFloat const textOffset= 60.;
  self.icon.origin = {leftOffset, topOffset};
  [self.textContainer setMinX:textOffset];
}

- (IBAction)cellTap
{
  switch (self.type)
  {
    case MWMPlacePageMetadataTypeURL:
    case MWMPlacePageMetadataTypeWebsite:
      [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatOpenSite)];
      break;
    case MWMPlacePageMetadataTypeEmail:
      [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatSendEmail)];
      break;
    case MWMPlacePageMetadataTypePhoneNumber:
      [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatCallPhoneNumber)];
      break;
    case MWMPlacePageMetadataTypeCoordinate:
      [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatToggleCoordinates)];
      break;
    default:
      break;
  }
  if (self.type != MWMPlacePageMetadataTypeCoordinate)
    return;
  NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
  BOOL const showLatLonAsDMS = [defaults boolForKey:kUserDefaultsLatLonAsDMSKey];
  m2::PointD const point = self.currentEntity.point;
  [self.textContainer setText:@((showLatLonAsDMS ? MeasurementUtils::FormatLatLon(point.x, point.y).c_str() : MeasurementUtils::FormatLatLonAsDMS(point.x, point.y, 2).c_str()))];
  [defaults setBool:!showLatLonAsDMS forKey:kUserDefaultsLatLonAsDMSKey];
  [defaults synchronize];
}

- (void)longTap:(UILongPressGestureRecognizer *)sender
{
  UIMenuController * menuController = [UIMenuController sharedMenuController];
  if (menuController.isMenuVisible)
    return;
  CGPoint const tapPoint = [sender locationInView:sender.view.superview];
  UIView * targetView = [self.textContainer isKindOfClass:[UITextView class]] ? sender.view : self.textContainer;
  [menuController setTargetRect:CGRectMake(tapPoint.x, targetView.minY, 0., 0.) inView:sender.view.superview];
  [menuController setMenuVisible:YES animated:YES];
  [targetView becomeFirstResponder];
  [menuController update];
}

@end
