#import "UIView+MPGoogleAdMobAdditions.h"

@implementation UIView (MPGoogleAdMobAdditions)
/// Adds constraints to the receiver's superview that keep the receiver centered in the superview.
- (void)gad_centerInSuperview {
  UIView *superview = self.superview;
  if (!superview) {
    return;
  }

  self.translatesAutoresizingMaskIntoConstraints = NO;

  [superview addConstraint:[NSLayoutConstraint constraintWithItem:self
                                                        attribute:NSLayoutAttributeCenterX
                                                        relatedBy:NSLayoutRelationEqual
                                                           toItem:superview
                                                        attribute:NSLayoutAttributeCenterX
                                                       multiplier:1
                                                         constant:0]];
  [superview addConstraint:[NSLayoutConstraint constraintWithItem:self
                                                        attribute:NSLayoutAttributeCenterY
                                                        relatedBy:NSLayoutRelationEqual
                                                           toItem:superview
                                                        attribute:NSLayoutAttributeCenterY
                                                       multiplier:1
                                                         constant:0]];
}

/// Adds constraints to the receiver's superview that keep the receiver the same size as the
/// superview.
- (void)gad_matchSuperviewSize {
  UIView *superview = self.superview;
  if (!superview) {
    return;
  }

  self.translatesAutoresizingMaskIntoConstraints = NO;

  [superview addConstraint:[NSLayoutConstraint constraintWithItem:self
                                                        attribute:NSLayoutAttributeWidth
                                                        relatedBy:NSLayoutRelationEqual
                                                           toItem:superview
                                                        attribute:NSLayoutAttributeWidth
                                                       multiplier:1
                                                         constant:0]];
  [superview addConstraint:[NSLayoutConstraint constraintWithItem:self
                                                        attribute:NSLayoutAttributeHeight
                                                        relatedBy:NSLayoutRelationEqual
                                                           toItem:superview
                                                        attribute:NSLayoutAttributeHeight
                                                       multiplier:1
                                                         constant:0]];
}

- (void)gad_fillSuperview {
  [self gad_centerInSuperview];
  [self gad_matchSuperviewSize];
}

@end
