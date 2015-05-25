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
@property (copy, nonatomic) NSString * type;

@end

@implementation MWMPlacePageInfoCell

- (void)configureWithIconTitle:(NSString *)title info:(NSString *)info
{
  self.type = title;
  [self.imageView setImage:[UIImage imageNamed:[NSString stringWithFormat:@"ic_%@", title]]];

  if ([self.textContainer isKindOfClass:[UITextView class]])
    [self.textContainer setAttributedText:[[NSAttributedString alloc] initWithString:info attributes:@{NSFontAttributeName : [UIFont fontWithName:@"HelveticaNeue-Light" size:16.]}]];
  else
    [self.textContainer setText:info];

  UILongPressGestureRecognizer * longTap = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(longTap:)];
  longTap.minimumPressDuration = 0.3f;
  [self.upperButton addGestureRecognizer:longTap];
}

- (BOOL)textView:(UITextView *)textView shouldInteractWithURL:(NSURL *)URL inRange:(NSRange)characterRange
{
  return YES;
}

- (IBAction)cellTap:(id)sender
{
  if ([self.type isEqualToString:@"Coordinate"])
  {
    NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
    BOOL const isLatLonAsDMA = [defaults boolForKey:kUserDefaultsLatLonAsDMSKey];
    m2::PointD const point = self.currentEntity.point;

    isLatLonAsDMA ? [self.textContainer setText:[NSString stringWithUTF8String:MeasurementUtils::FormatLatLon(point.x, point.y).c_str()]] : [self.textContainer setText:[NSString stringWithUTF8String:MeasurementUtils::FormatLatLonAsDMS(point.x, point.y, 2).c_str()]];

    [defaults setBool:!isLatLonAsDMA forKey:kUserDefaultsLatLonAsDMSKey];
    [defaults synchronize];
  }
}

- (void)longTap:(UILongPressGestureRecognizer *)sender
{
  UIMenuController * menuController = [UIMenuController sharedMenuController];
  if (menuController.isMenuVisible)
    return;

  CGPoint tapPoint = [sender locationInView:sender.view.superview];
  UIView * targetView = [self.textContainer isKindOfClass:[UITextView class]] ? sender.view : self.textContainer;
  [menuController setTargetRect:CGRectMake(tapPoint.x, targetView.minY, 0., 0.) inView:sender.view.superview];
  [menuController setMenuVisible:YES animated:YES];
  [targetView becomeFirstResponder];
  [menuController update];
}

@end
