#import <CoreApi/MWMCommon.h>
#import "MWMSearchManager+Layout.h"
#import "MapViewController.h"

static CGFloat const kWidthForiPad = 320.0;
static CGFloat const changeModeViewOffsetNormal = -24;
static CGFloat const changeModeViewOffsetKeyboard = -12;

@interface MWMSearchManager ()

@property(nonatomic) IBOutlet UIView *searchBarView;
@property(nonatomic) IBOutlet UIView *actionBarView;
@property(nonatomic) IBOutlet UIView *contentView;

@property(nonatomic) NSLayoutConstraint *contentViewTopHidden;
@property(nonatomic) NSLayoutConstraint *contentViewBottomHidden;
@property(nonatomic) NSLayoutConstraint *actionBarViewBottomKeyboard;
@property(nonatomic) NSLayoutConstraint *actionBarViewBottomNormal;

@property(weak, nonatomic, readonly) UIView *searchViewContainer;

@end

@implementation MWMSearchManager (Layout)

- (void)layoutTopViews {
  UIView *searchBarView = self.searchBarView;
  UIView *changeModeView = self.actionBarView;
  UIView *contentView = self.contentView;
  UIView *parentView = self.searchViewContainer;

  searchBarView.translatesAutoresizingMaskIntoConstraints = NO;
  changeModeView.translatesAutoresizingMaskIntoConstraints = NO;
  contentView.translatesAutoresizingMaskIntoConstraints = NO;

  NSLayoutXAxisAnchor *leadingAnchor = parentView.leadingAnchor;
  NSLayoutXAxisAnchor *trailingAnchor = parentView.trailingAnchor;
  NSLayoutYAxisAnchor *topAnchor = parentView.safeAreaLayoutGuide.topAnchor;
  NSLayoutYAxisAnchor *bottomAnchor = parentView.safeAreaLayoutGuide.bottomAnchor;

  [searchBarView.topAnchor constraintEqualToAnchor:topAnchor].active = YES;
  [searchBarView.leadingAnchor constraintEqualToAnchor:leadingAnchor].active = YES;
  if (IPAD)
    [searchBarView.widthAnchor constraintEqualToConstant:kWidthForiPad].active = YES;
  else
    [searchBarView.trailingAnchor constraintEqualToAnchor:trailingAnchor].active = YES;

  [changeModeView.centerXAnchor constraintEqualToAnchor:parentView.centerXAnchor].active = YES;
  self.actionBarViewBottomNormal = [changeModeView.bottomAnchor constraintEqualToAnchor:bottomAnchor
                                                                               constant:changeModeViewOffsetNormal];
  self.actionBarViewBottomNormal.priority = UILayoutPriorityDefaultLow + 10;
  self.actionBarViewBottomNormal.active = YES;

  self.actionBarViewBottomKeyboard = [changeModeView.bottomAnchor constraintEqualToAnchor:parentView.bottomAnchor
                                                                                 constant:changeModeViewOffsetKeyboard];
  self.actionBarViewBottomKeyboard.priority = UILayoutPriorityDefaultLow;
  self.actionBarViewBottomKeyboard.active = YES;

  NSLayoutConstraint *contentViewTop = [contentView.topAnchor constraintEqualToAnchor:searchBarView.bottomAnchor];
  contentViewTop.priority = UILayoutPriorityDefaultLow + 10;
  contentViewTop.active = YES;

  NSLayoutConstraint *contentViewBottom = [contentView.bottomAnchor constraintEqualToAnchor:parentView.bottomAnchor];
  contentViewBottom.priority = UILayoutPriorityDefaultLow + 10;
  contentViewBottom.active = YES;

  self.contentViewTopHidden = [contentView.topAnchor constraintEqualToAnchor:parentView.bottomAnchor];
  self.contentViewTopHidden.priority = UILayoutPriorityDefaultLow;
  self.contentViewTopHidden.active = YES;

  self.contentViewBottomHidden = [contentView.heightAnchor constraintEqualToAnchor:parentView.heightAnchor];
  self.contentViewBottomHidden.priority = UILayoutPriorityDefaultLow;
  self.contentViewBottomHidden.active = YES;

  [contentView.leadingAnchor constraintEqualToAnchor:searchBarView.leadingAnchor].active = YES;
  [contentView.trailingAnchor constraintEqualToAnchor:searchBarView.trailingAnchor].active = YES;

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillShow:)
                                               name:UIKeyboardWillShowNotification
                                             object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillHide:)
                                               name:UIKeyboardWillHideNotification
                                             object:nil];
}

- (void)removeKeyboardObservers {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - keyboard movements
- (void)keyboardWillShow:(NSNotification *)notification {
  CGSize keyboardSize = [[[notification userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue].size;
  CGFloat offset = IPAD ? changeModeViewOffsetNormal : changeModeViewOffsetKeyboard;
  if (self.actionBarView.isHidden) {
    self.actionBarViewBottomKeyboard.constant = -keyboardSize.height + offset;
    self.actionBarViewBottomKeyboard.priority = UILayoutPriorityDefaultHigh;
  } else {
    [UIView animateWithDuration:kDefaultAnimationDuration
                     animations:^{
                       self.actionBarViewBottomKeyboard.constant = -keyboardSize.height + offset;
                       self.actionBarViewBottomKeyboard.priority = UILayoutPriorityDefaultHigh;
                       [self.actionBarView.superview layoutIfNeeded];
                     }];
  }
}

- (void)keyboardWillHide:(NSNotification *)notification {
  if (self.actionBarView.isHidden) {
    self.actionBarViewBottomKeyboard.priority = UILayoutPriorityDefaultLow;
  } else {
    [UIView animateWithDuration:kDefaultAnimationDuration
                     animations:^{
                       self.actionBarViewBottomKeyboard.priority = UILayoutPriorityDefaultLow;
                       [self.actionBarView.superview layoutIfNeeded];
                     }];
  }
}

@end
