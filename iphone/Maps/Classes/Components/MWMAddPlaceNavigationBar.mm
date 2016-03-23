#import "Common.h"
#import "MWMAddPlaceNavigationBar.h"

#include "Framework.h"

@interface MWMAddPlaceNavigationBar ()

@property (copy, nonatomic) TMWMVoidBlock doneBlock;
@property (copy, nonatomic) TMWMVoidBlock cancelBlock;

@end

@implementation MWMAddPlaceNavigationBar

+ (void)showInSuperview:(UIView *)superview doneBlock:(TMWMVoidBlock)done cancelBlock:(TMWMVoidBlock)cancel
{
  MWMAddPlaceNavigationBar * navBar = [[[NSBundle mainBundle] loadNibNamed:self.className owner:nil options:nil] firstObject];
  navBar.width = superview.width;
  navBar.doneBlock = done;
  navBar.cancelBlock = cancel;
  navBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  [navBar setNeedsLayout];
  navBar.origin = {0., -navBar.height};
  [superview addSubview:navBar];
  [navBar show];
}

- (void)show
{
  auto & f = GetFramework();
  f.EnableChoosePositionMode(true);
  f.BlockTapEvents(true);

  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.transform = CGAffineTransformMakeTranslation(0., self.height);
  }];
}

- (void)dismiss
{
  auto & f = GetFramework();
  f.EnableChoosePositionMode(false);
  f.BlockTapEvents(false);

  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.transform = CGAffineTransformMakeTranslation(0., -self.height);
  }
  completion:^(BOOL finished)
  {
    [self removeFromSuperview];
  }];
}

- (IBAction)doneTap
{
  [self dismiss];
  self.doneBlock();
}

- (IBAction)cancelTap
{
  [self dismiss];
  self.cancelBlock();
}

- (void)layoutSubviews
{
  if (self.superview)
    self.width = self.superview.width;
  [super layoutSubviews];
}

@end
