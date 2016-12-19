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
  NSLayoutConstraint * searchBarViewTop =
      [NSLayoutConstraint constraintWithItem:searchBarView
                                   attribute:NSLayoutAttributeTop
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:parentView
                                   attribute:NSLayoutAttributeTop
                                  multiplier:1
                                    constant:statusBarHeight()];
  NSLayoutConstraint * searchBarViewLeft =
      [NSLayoutConstraint constraintWithItem:searchBarView
                                   attribute:NSLayoutAttributeLeft
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:parentView
                                   attribute:NSLayoutAttributeLeft
                                  multiplier:1
                                    constant:0];
  NSLayoutConstraint * actionBarViewTop =
      [NSLayoutConstraint constraintWithItem:actionBarView
                                   attribute:NSLayoutAttributeTop
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:searchBarView
                                   attribute:NSLayoutAttributeBottom
                                  multiplier:1
                                    constant:0];
  actionBarViewTop.priority = UILayoutPriorityDefaultLow + 10;
  NSLayoutConstraint * actionBarViewLeft =
      [NSLayoutConstraint constraintWithItem:actionBarView
                                   attribute:NSLayoutAttributeLeft
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:parentView
                                   attribute:NSLayoutAttributeLeft
                                  multiplier:1
                                    constant:0];
  NSLayoutConstraint * actionBarViewWidth =
      [NSLayoutConstraint constraintWithItem:actionBarView
                                   attribute:NSLayoutAttributeWidth
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:searchBarView
                                   attribute:NSLayoutAttributeWidth
                                  multiplier:1
                                    constant:0];
  self.actionBarViewBottom = [NSLayoutConstraint constraintWithItem:actionBarView
                                                          attribute:NSLayoutAttributeBottom
                                                          relatedBy:NSLayoutRelationEqual
                                                             toItem:parentView
                                                          attribute:NSLayoutAttributeBottom
                                                         multiplier:1
                                                           constant:0];
  self.actionBarViewBottom.priority = UILayoutPriorityDefaultLow;

  NSLayoutConstraint * contentViewTop =
      [NSLayoutConstraint constraintWithItem:contentView
                                   attribute:NSLayoutAttributeTop
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:actionBarView
                                   attribute:NSLayoutAttributeBottom
                                  multiplier:1
                                    constant:0];
  contentViewTop.priority = UILayoutPriorityDefaultLow;
  NSLayoutConstraint * contentViewBottom =
      [NSLayoutConstraint constraintWithItem:contentView
                                   attribute:NSLayoutAttributeBottom
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:parentView
                                   attribute:NSLayoutAttributeBottom
                                  multiplier:1
                                    constant:0];
  contentViewBottom.priority = UILayoutPriorityDefaultLow + 10;
  NSLayoutConstraint * contentViewLeft =
      [NSLayoutConstraint constraintWithItem:contentView
                                   attribute:NSLayoutAttributeLeft
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:parentView
                                   attribute:NSLayoutAttributeLeft
                                  multiplier:1
                                    constant:0];
  NSLayoutConstraint * contentViewWidth =
      [NSLayoutConstraint constraintWithItem:contentView
                                   attribute:NSLayoutAttributeWidth
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:searchBarView
                                   attribute:NSLayoutAttributeWidth
                                  multiplier:1
                                    constant:0];

  [parentView addConstraints:@[
    searchBarViewTop, searchBarViewLeft, actionBarViewTop, actionBarViewLeft, actionBarViewWidth,
    self.actionBarViewBottom, contentViewTop, contentViewLeft, contentViewWidth, contentViewBottom
  ]];

  if (IPAD)
  {
    NSLayoutConstraint * searchBarViewWidth =
        [NSLayoutConstraint constraintWithItem:searchBarView
                                     attribute:NSLayoutAttributeWidth
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:nil
                                     attribute:NSLayoutAttributeNotAnAttribute
                                    multiplier:1
                                      constant:kWidthForiPad];
    [parentView addConstraint:searchBarViewWidth];
  }
  else
  {
    NSLayoutConstraint * searchBarViewRight =
        [NSLayoutConstraint constraintWithItem:searchBarView
                                     attribute:NSLayoutAttributeRight
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:parentView
                                     attribute:NSLayoutAttributeRight
                                    multiplier:1
                                      constant:0];
    [parentView addConstraint:searchBarViewRight];
  }
}

@end
