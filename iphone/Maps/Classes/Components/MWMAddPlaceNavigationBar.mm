#import "MWMAddPlaceNavigationBar.h"
#import "MWMCommon.h"

#include "Framework.h"

@interface MWMAddPlaceNavigationBar ()

@property(copy, nonatomic) MWMVoidBlock doneBlock;
@property(copy, nonatomic) MWMVoidBlock cancelBlock;

@end

@implementation MWMAddPlaceNavigationBar

+ (void)showInSuperview:(UIView *)superview
             isBusiness:(BOOL)isBusiness
          applyPosition:(BOOL)applyPosition
               position:(m2::PointD const &)position
              doneBlock:(MWMVoidBlock)done
            cancelBlock:(MWMVoidBlock)cancel
{
  MWMAddPlaceNavigationBar * navBar =
      [NSBundle.mainBundle loadNibNamed:self.className owner:nil options:nil].firstObject;
  navBar.width = superview.width;
  navBar.doneBlock = done;
  navBar.cancelBlock = cancel;
  navBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  [navBar setNeedsLayout];
  navBar.origin = {0., -navBar.height};
  [superview addSubview:navBar];
  [navBar show:isBusiness applyPosition:applyPosition position:position];
}

- (void)show:(BOOL)enableBounds applyPosition:(BOOL)applyPosition position:(m2::PointD const &)position
{
  auto & f = GetFramework();
  f.EnableChoosePositionMode(true /* enable */, enableBounds, applyPosition, position);
  f.BlockTapEvents(true);

  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.transform = CGAffineTransformMakeTranslation(0., self.height);
  }];
}

- (void)dismissWithBlock:(MWMVoidBlock)block
{
  auto & f = GetFramework();
  f.EnableChoosePositionMode(false /* enable */, false /* enableBounds */, false /* applyPosition */, m2::PointD());
  f.BlockTapEvents(false);

  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.transform = CGAffineTransformMakeTranslation(0., -self.height);
  }
  completion:^(BOOL finished)
  {
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

- (void)layoutSubviews
{
  if (self.superview)
    self.width = self.superview.width;
  [super layoutSubviews];
}

@end
