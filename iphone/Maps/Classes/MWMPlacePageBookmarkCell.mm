//
//  MWMPlacePageBookmarkCell.m
//  Maps
//
//  Created by v.mikhaylenko on 29.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePageBookmarkCell.h"
#import "UIKitCategories.h"

@interface MWMPlacePageBookmarkCell () <UITextViewDelegate>

@property (weak, nonatomic, readwrite) IBOutlet UITextView * title;
@property (weak, nonatomic, readwrite) IBOutlet UIButton * categoryButton;
@property (weak, nonatomic, readwrite) IBOutlet UIButton * markButton;
@property (weak, nonatomic, readwrite) IBOutlet UITextView * descriptionTextView;

@property (weak, nonatomic) IBOutlet UIView * firstSeparatorView;
@property (weak, nonatomic) IBOutlet UIView * secondSeparatorView;

@property (nonatomic) UITextView * activeTextView;

@end

@implementation MWMPlacePageBookmarkCell

- (void)configure
{
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWasShown:)
                                               name:UIKeyboardDidShowNotification object:nil];

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillBeHidden:)
                                               name:UIKeyboardWillHideNotification object:nil];
}

- (void)keyboardWasShown:(NSNotification *)aNotification
{
  NSDictionary const * info = [aNotification userInfo];
  CGSize const kbSize = [[info objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;

  UIEdgeInsets const contentInsets = UIEdgeInsetsMake(0., 0., kbSize.height, 0.);
  self.ownerTableView.contentInset = contentInsets;
  self.ownerTableView.scrollIndicatorInsets = contentInsets;

  CGRect aRect = self.ownerTableView.superview.frame;
  aRect.size.height -= kbSize.height;
  if (!CGRectContainsPoint(aRect, self.activeTextView.frame.origin))
    [self.ownerTableView scrollRectToVisible:self.activeTextView.frame animated:YES];
}

- (void)keyboardWillBeHidden:(NSNotification *)aNotification
{
  UIEdgeInsets const contentInsets = UIEdgeInsetsZero;
  self.ownerTableView.contentInset = contentInsets;
  self.ownerTableView.scrollIndicatorInsets = contentInsets;
}

- (void)textViewDidBeginEditing:(UITextView *)textView
{
  self.activeTextView = textView;
}

- (void)textViewDidEndEditing:(UITextView *)textView
{
  self.activeTextView = nil;
}

- (void)textViewDidChange:(UITextView *)textView
{
  [textView scrollRangeToVisible:[textView selectedRange]];
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
