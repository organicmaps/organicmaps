//
//  MWMPlacePageBookmarkCell.m
//  Maps
//
//  Created by v.mikhaylenko on 29.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePageBookmarkCell.h"
#import "MWMPlacePageEntity.h"
#import "UIKitCategories.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageViewManager.h"
#import "MWMBasePlacePageView.h"
#import "MWMTextView.h"

extern NSString * const kBookmarkCellWebViewDidFinishLoadContetnNotification = @"WebViewDidFifishLoadContent";

@interface MWMPlacePageBookmarkCell () <UITextFieldDelegate, UIWebViewDelegate>

@property (weak, nonatomic, readwrite) IBOutlet UITextField * title;
@property (weak, nonatomic, readwrite) IBOutlet UIButton * categoryButton;
@property (weak, nonatomic, readwrite) IBOutlet UIButton * markButton;
@property (weak, nonatomic, readwrite) IBOutlet UILabel * descriptionLabel;
@property (weak, nonatomic) IBOutlet UIImageView * icon;

@property (weak, nonatomic) IBOutlet UIView * firstSeparatorView;
@property (weak, nonatomic) IBOutlet UIView * secondSeparatorView;

@end

@implementation MWMPlacePageBookmarkCell

- (void)configure
{
  MWMPlacePageEntity const * entity = self.placePage.manager.entity;
  self.title.text = entity.bookmarkTitle;
  [self.categoryButton setTitle:[NSString stringWithFormat:@"%@ >", entity.bookmarkCategory] forState:UIControlStateNormal];
  [self.markButton setImage:[UIImage imageNamed:[NSString stringWithFormat:@"%@-on", entity.bookmarkColor]] forState:UIControlStateNormal];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillShown:)
                                               name:UIKeyboardWillShowNotification object:nil];

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillBeHidden)
                                               name:UIKeyboardWillHideNotification object:nil];
}

- (void)keyboardWillShown:(NSNotification *)aNotification
{
  NSDictionary const * info = [aNotification userInfo];
  CGSize const kbSize = [[info objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
  if ([self.title isEditing])
    [self.placePage willStartEditingBookmarkTitle:kbSize.height];
}

- (void)keyboardWillBeHidden
{
  if ([self.title isEditing])
    [self.placePage willFinishEditingBookmarkTitle:self.title.text.length > 0 ? self.title.text : self.placePage.manager.entity.title];
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
  MWMPlacePageEntity * entity = self.placePage.manager.entity;
  entity.bookmarkTitle = textField.text.length > 0 ? textField.text : self.placePage.manager.entity.title;
  [entity synchronize];
  [textField resignFirstResponder];
}

- (void)layoutSubviews
{
  CGFloat const leftOffset = 16.;
  CGFloat const topOffset = 14.;
  self.icon.origin = CGPointMake(leftOffset, topOffset);
}

- (BOOL)textFieldShouldClear:(UITextField *)textField
{
  return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  [textField resignFirstResponder];
  return YES;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (IBAction)colorPickerButtonTap
{
  [self.placePage changeBookmarkColor];
  [self.title resignFirstResponder];
}

- (IBAction)categoryButtonTap
{
  [self.placePage changeBookmarkCategory];
  [self.title resignFirstResponder];
}

- (IBAction)editTap
{
  [self.placePage changeBookmarkDescription];
  [self.title resignFirstResponder];
}

@end
