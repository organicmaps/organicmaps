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
                                           selector:@selector(keyboardWillBeHidden:)
                                               name:UIKeyboardWillHideNotification object:nil];
}

- (void)keyboardWillShown:(NSNotification *)aNotification
{

  NSDictionary const * info = [aNotification userInfo];
  CGSize const kbSize = [[info objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
  [self.placePage willStartEditingBookmarkTitle:kbSize.height];
  
//  UIEdgeInsets const contentInsets = UIEdgeInsetsMake(0., 0., kbSize.height, 0.);
//  self.ownerTableView.contentInset = contentInsets;
//  self.ownerTableView.scrollIndicatorInsets = contentInsets;
//
//  CGRect aRect = self.ownerTableView.superview.frame;
//  aRect.size.height -= kbSize.height;
//  if (!CGRectContainsPoint(aRect, self.title.frame.origin))
//    [self.ownerTableView scrollRectToVisible:aRect animated:YES];
}

- (void)keyboardWillBeHidden:(NSNotification *)aNotification
{
  NSDictionary const * info = [aNotification userInfo];
  CGSize const kbSize = [[info objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
  [self.placePage willFinishEditingBookmarkTitle:kbSize.height];
//  UIEdgeInsets const contentInsets = UIEdgeInsetsZero;
//  self.ownerTableView.contentInset = contentInsets;
//  self.ownerTableView.scrollIndicatorInsets = contentInsets;
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
  MWMPlacePageEntity * entity = self.placePage.manager.entity;
  entity.bookmarkTitle = textField.text;
  [entity synchronize];
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

- (IBAction)colorPickerButtonTap:(id)sender
{
  [self.placePage changeBookmarkColor];
}

- (IBAction)categoryButtonTap:(id)sender
{
  [self.placePage changeBookmarkCategory];
}

- (IBAction)editTap:(id)sender
{
  [self.placePage changeBookmarkDescription];
}

@end
