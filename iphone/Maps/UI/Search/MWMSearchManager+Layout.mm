#import "MWMCommon.h"
#import "MWMSearchManager+Layout.h"

namespace
{
CGFloat const kWidthForiPad = 320.0;
}  // namespace

@interface MWMSearchManager ()

@property(nonatomic) IBOutlet UIView * searchBarView;
@property(nonatomic) IBOutlet UIView * actionBarView;
@property(nonatomic) IBOutlet UIView * contentView;

@property(nonatomic) NSLayoutConstraint * actionBarViewBottom;

@property(weak, nonatomic, readonly) UIViewController * ownerController;

@end

@implementation MWMSearchManager (Layout)

- (void)layoutTopViews
{
  UIView * searchBarView = self.searchBarView;
  UIView * actionBarView = self.actionBarView;
  UIView * contentView = self.contentView;
  UIView * parentView = self.ownerController.view;

  searchBarView.translatesAutoresizingMaskIntoConstraints = NO;
  actionBarView.translatesAutoresizingMaskIntoConstraints = NO;
  contentView.translatesAutoresizingMaskIntoConstraints = NO;

  NSLayoutXAxisAnchor * leadingAnchor = parentView.leadingAnchor;
  NSLayoutXAxisAnchor * trailingAnchor = parentView.trailingAnchor;
  NSLayoutYAxisAnchor * topAnchor = parentView.topAnchor;
  NSLayoutYAxisAnchor * bottomAnchor = parentView.bottomAnchor;
  CGFloat topOffset = 0;
  if (@available(iOS 11.0, *))
  {
    UILayoutGuide * safeAreaLayoutGuide = parentView.safeAreaLayoutGuide;
    leadingAnchor = safeAreaLayoutGuide.leadingAnchor;
    trailingAnchor = safeAreaLayoutGuide.trailingAnchor;
    topAnchor = safeAreaLayoutGuide.topAnchor;
    bottomAnchor = safeAreaLayoutGuide.bottomAnchor;
  }
  else
  {
    topOffset = statusBarHeight();
  }

  [searchBarView.topAnchor constraintEqualToAnchor:topAnchor constant:topOffset].active = YES;
  [searchBarView.leadingAnchor constraintEqualToAnchor:leadingAnchor].active = YES;
  if (IPAD)
    [searchBarView.widthAnchor constraintEqualToConstant:kWidthForiPad].active = YES;
  else
    [searchBarView.trailingAnchor constraintEqualToAnchor:trailingAnchor].active = YES;

  NSLayoutConstraint * actionBarViewTop =
      [actionBarView.topAnchor constraintEqualToAnchor:searchBarView.bottomAnchor];
  actionBarViewTop.priority = UILayoutPriorityDefaultLow + 10;
  actionBarViewTop.active = YES;

  [actionBarView.leadingAnchor constraintEqualToAnchor:searchBarView.leadingAnchor].active = YES;
  [actionBarView.trailingAnchor constraintEqualToAnchor:searchBarView.trailingAnchor].active = YES;
  self.actionBarViewBottom = [actionBarView.bottomAnchor constraintEqualToAnchor:bottomAnchor];
  self.actionBarViewBottom.priority = UILayoutPriorityDefaultLow;
  self.actionBarViewBottom.active = YES;

  NSLayoutConstraint * contentViewTop =
      [contentView.topAnchor constraintEqualToAnchor:actionBarView.bottomAnchor];
  contentViewTop.priority = UILayoutPriorityDefaultLow;
  contentViewTop.active = YES;

  NSLayoutConstraint * contentViewBottom =
      [contentView.bottomAnchor constraintEqualToAnchor:bottomAnchor];
  contentViewBottom.priority = UILayoutPriorityDefaultLow + 10;
  contentViewBottom.active = YES;

  [contentView.leadingAnchor constraintEqualToAnchor:searchBarView.leadingAnchor].active = YES;
  [contentView.trailingAnchor constraintEqualToAnchor:searchBarView.trailingAnchor].active = YES;
}

@end
