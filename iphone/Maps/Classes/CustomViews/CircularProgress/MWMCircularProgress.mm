#import "MWMCircularProgress.h"
#import "MWMCircularProgressView.h"

@interface MWMCircularProgress ()

@property (nonatomic) IBOutlet MWMCircularProgressView * rootView;
@property (nonatomic) IBOutlet UIButton * button;

@property (nonatomic) NSNumber * nextProgressToAnimate;

@property (weak, nonatomic) id <MWMCircularProgressDelegate> delegate;

@end

@implementation MWMCircularProgress

- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView delegate:(nonnull id <MWMCircularProgressDelegate>)delegate
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:self.class.className owner:self options:nil];
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

- (void)setImage:(nullable UIImage *)image forState:(UIControlState)state
{
  [self.button setImage:image forState:state];
}

#pragma mark - Spinner

- (void)startSpinner
{
  [self.rootView startSpinner];
}

- (void)stopSpinner
{
  [self.rootView stopSpinner];
}

#pragma mark - Animation

- (void)animationDidStop:(CABasicAnimation *)anim finished:(BOOL)flag
{
  if (self.nextProgressToAnimate)
  {
    self.progress = self.nextProgressToAnimate.doubleValue;
    self.nextProgressToAnimate = nil;
  }
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

- (void)setSelected:(BOOL)selected
{
  self.button.selected = selected;
}

- (BOOL)selected
{
  return self.button.selected;
}

@end
