//
//  MWMPlacePageBaseCell.m
//  Maps
//
//  Created by v.mikhaylenko on 27.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePageInfoCell.h"
#import "UIKitCategories.h"
#import "MWMPlacePageEntity.h"

#include "../../../platform/settings.hpp"
#include "../../../platform/measurement_utils.hpp"

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
  NSMutableString * imageName = [@"ic_" mutableCopy];
  switch (type)
  {
    case MWMPlacePageMetadataTypeURL:
    case MWMPlacePageMetadataTypeWebsite:
      [imageName appendString:@"Website"];
      break;
    case MWMPlacePageMetadataTypeEmail:
      [imageName appendString:@"Email"];
      break;
    case MWMPlacePageMetadataTypePhoneNumber:
      [imageName appendString:@"PhoneNumber"];
      break;
    case MWMPlacePageMetadataTypeCoordinate:
      [imageName appendString:@"Coordinate"];
      break;
    case MWMPlacePageMetadataTypePostcode:
      [imageName appendString:@"Postcode"];
      break;
    case MWMPlacePageMetadataTypeOpenHours:
      [imageName appendString:@"OpenHours"];
      break;
    case MWMPlacePageMetadataTypeBookmark:
      NSAssert(false, @"Incorrect type!");
      break;
  }
  
  UIImage * image = [UIImage imageNamed:imageName];
  self.type = type;
  [self.imageView setImage:image];

  if ([self.textContainer isKindOfClass:[UITextView class]])
    [self.textContainer setAttributedText:[[NSAttributedString alloc] initWithString:info attributes:@{NSFontAttributeName : [UIFont fontWithName:@"HelveticaNeue-Light" size:16.]}]];
  else
    [self.textContainer setText:info];

  UILongPressGestureRecognizer * longTap = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(longTap:)];
  longTap.minimumPressDuration = 0.3;
  [self.upperButton addGestureRecognizer:longTap];
}

- (BOOL)textView:(UITextView *)textView shouldInteractWithURL:(NSURL *)URL inRange:(NSRange)characterRange
{
  return YES;
}

- (IBAction)cellTap
{
  if (self.type != MWMPlacePageMetadataTypeCoordinate)
    return;

  NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
  BOOL const showLatLonAsDMS = [defaults boolForKey:kUserDefaultsLatLonAsDMSKey];
  m2::PointD const point = self.currentEntity.point;
  [self.textContainer setText:[NSString stringWithUTF8String:(showLatLonAsDMS ? MeasurementUtils::FormatLatLon(point.x, point.y).c_str() : MeasurementUtils::FormatLatLonAsDMS(point.x, point.y, 2).c_str())]];
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
