#import "MWMCircularProgress.h"
#import "MWMCircularProgressView.h"

@interface MWMCircularProgress ()

@property (nonatomic) IBOutlet MWMCircularProgressView * rootView;
@property (nonatomic) IBOutlet UIButton * button;

@property (nonatomic) NSNumber * nextProgressToAnimate;

@property (nonnull, weak, nonatomic) id <MWMCircularProgressDelegate> delegate;

@end

@implementation MWMCircularProgress

- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView delegate:(nonnull id <MWMCircularProgressDelegate>)delegate
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
    [parentView addSubview:self.rootView];
    self.delegate = delegate;
    [self reset];
  }
  return self;
}

- (void)reset
{
  [self.rootView updatePath:0.];
  self.nextProgressToAnimate = nil;
}

#pragma mark - Animation

- (void)animationDidStop:(CABasicAnimation *)anim finished:(BOOL)flag
{
  if (self.nextProgressToAnimate)
    self.progress = self.nextProgressToAnimate.doubleValue;
}

#pragma mark - Actions

- (IBAction)buttonTouchUpInside:(UIButton *)sender
{
  [self.delegate progressButtonPressed:self];
}

#pragma mark - Properties

- (void)setProgress:(CGFloat)progress
{
  if (self.rootView.animating)
  {
    self.nextProgressToAnimate = @(progress);
  }
  else
  {
    [self.rootView animateFromValue:_progress toValue:progress];
    _progress = progress;
  }
}

- (void)setFailed:(BOOL)failed
{
  if (self.button.selected == failed)
    return;
  self.button.selected = failed;
  [self.rootView refreshProgress];
  [self.rootView updatePath:self.progress];
}

- (BOOL)failed
{
  return self.button.selected;
}

@end
