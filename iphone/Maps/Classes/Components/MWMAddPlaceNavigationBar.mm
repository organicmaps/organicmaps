#import "MWMAddPlaceNavigationBar.h"

#include <CoreApi/Framework.h>

@interface MWMAddPlaceNavigationBar ()

@property(copy, nonatomic) MWMVoidBlock doneBlock;
@property(copy, nonatomic) MWMVoidBlock cancelBlock;
@property(assign, nonatomic) NSLayoutConstraint * topConstraint;

@end

@implementation MWMAddPlaceNavigationBar

+ (void)showInSuperview:(UIView *)superview
             isBusiness:(BOOL)isBusiness
               position:(m2::PointD const *)optionalPosition
              doneBlock:(MWMVoidBlock)done
            cancelBlock:(MWMVoidBlock)cancel
{
  MWMAddPlaceNavigationBar * navBar =
      [NSBundle.mainBundle loadNibNamed:self.className owner:nil options:nil].firstObject;
  navBar.width = superview.width;
  navBar.doneBlock = done;
  navBar.cancelBlock = cancel;
  navBar.translatesAutoresizingMaskIntoConstraints = false;

  [superview addSubview:navBar];
  navBar.topConstraint = [navBar.topAnchor constraintEqualToAnchor:superview.topAnchor];
  navBar.topConstraint.active = true;
  navBar.topConstraint.constant = -navBar.height;
  [navBar.trailingAnchor constraintEqualToAnchor:superview.trailingAnchor].active = true;
  [navBar.leadingAnchor constraintEqualToAnchor:superview.leadingAnchor].active = true;
  [navBar show:isBusiness position:optionalPosition];
}

- (void)show:(BOOL)enableBounds position:(m2::PointD const *)optionalPosition
{
  auto & f = GetFramework();
  f.EnableChoosePositionMode(true /* enable */, enableBounds, optionalPosition);
  f.BlockTapEvents(true);

  [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ self.topConstraint.constant = 0; }];
}

- (void)dismissWithBlock:(MWMVoidBlock)block
{
  auto & f = GetFramework();
  f.EnableChoosePositionMode(false /* enable */, false /* enableBounds */, nullptr /* optionalPosition */);
  f.BlockTapEvents(false);

  [UIView animateWithDuration:kDefaultAnimationDuration
      animations:^{ self.topConstraint.constant = -self.height; }
      completion:^(BOOL finished) {
        [self removeFromSuperview];
        block();
      }];
}

- (IBAction)doneTap
{
  [self dismissWithBlock:self.doneBlock];
}

- (IBAction)cancelTap
{
  [self dismissWithBlock:self.cancelBlock];
}

@end
