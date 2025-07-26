#import "MWMCircularProgress.h"
#import "MWMCircularProgressView.h"

@interface MWMCircularProgressView ()

@property(nonatomic) BOOL suspendRefreshProgress;

@end

@interface MWMCircularProgress ()

@property(nonatomic) IBOutlet MWMCircularProgressView * rootView;
@property(nonatomic) NSNumber * nextProgressToAnimate;

@end

@implementation MWMCircularProgress

+ (nonnull instancetype)downloaderProgressForParentView:(nonnull UIView *)parentView
{
  MWMCircularProgress * progress = [[self alloc] initWithParentView:parentView];

  progress.rootView.suspendRefreshProgress = YES;

  [progress setImageName:@"ic_download"
               forStates:@[@(MWMCircularProgressStateNormal), @(MWMCircularProgressStateSelected)]];
  [progress setImageName:@"ic_close_spinner"
               forStates:@[@(MWMCircularProgressStateProgress), @(MWMCircularProgressStateSpinner)]];
  [progress setImageName:@"ic_download_error" forStates:@[@(MWMCircularProgressStateFailed)]];
  [progress setImageName:@"ic_check" forStates:@[@(MWMCircularProgressStateCompleted)]];

  [progress setColoring:MWMButtonColoringBlack
              forStates:@[
                @(MWMCircularProgressStateNormal), @(MWMCircularProgressStateSelected),
                @(MWMCircularProgressStateProgress), @(MWMCircularProgressStateSpinner)
              ]];
  [progress setColoring:MWMButtonColoringOther forStates:@[@(MWMCircularProgressStateFailed)]];
  [progress setColoring:MWMButtonColoringBlue forStates:@[@(MWMCircularProgressStateCompleted)]];

  progress.rootView.suspendRefreshProgress = NO;

  return progress;
}

- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView
{
  self = [super init];
  if (self)
  {
    [[UINib nibWithNibName:NSStringFromClass([self class]) bundle:nil] instantiateWithOwner:self options:nil];
    [parentView.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
    [parentView addSubview:self.rootView];
    self.rootView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.rootView.topAnchor constraintEqualToAnchor:parentView.topAnchor].active = YES;
    [self.rootView.bottomAnchor constraintEqualToAnchor:parentView.bottomAnchor].active = YES;
    [self.rootView.leadingAnchor constraintEqualToAnchor:parentView.leadingAnchor].active = YES;
    [self.rootView.trailingAnchor constraintEqualToAnchor:parentView.trailingAnchor].active = YES;
    self.state = MWMCircularProgressStateNormal;
  }
  return self;
}

- (void)dealloc
{
  [self.rootView removeFromSuperview];
}
- (void)reset
{
  _progress = 0.;
  [self.rootView updatePath:0.];
  self.nextProgressToAnimate = nil;
}

- (void)setSpinnerColoring:(MWMImageColoring)coloring
{
  [self.rootView setSpinnerColoring:coloring];
}

- (void)setSpinnerBackgroundColor:(nonnull UIColor *)backgroundColor
{
  [self.rootView setSpinnerBackgroundColor:backgroundColor];
}

- (void)setImageName:(nullable NSString *)imageName forStates:(MWMCircularProgressStateVec)states
{
  for (NSNumber * state in states)
    [self.rootView setImageName:imageName forState:(MWMCircularProgressState)state.integerValue];
}

- (void)setColor:(UIColor *)color forStates:(MWMCircularProgressStateVec)states
{
  for (NSNumber * state in states)
    [self.rootView setColor:color forState:(MWMCircularProgressState)state.integerValue];
}

- (void)setColoring:(MWMButtonColoring)coloring forStates:(MWMCircularProgressStateVec)states
{
  for (NSNumber * state in states)
    [self.rootView setColoring:coloring forState:(MWMCircularProgressState)state.integerValue];
}

- (void)setInvertColor:(BOOL)invertColor
{
  self.rootView.isInvertColor = invertColor;
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

- (void)setCancelButtonHidden
{
  self.rootView.buttonView.hidden = YES;
}

- (IBAction)buttonTouchUpInside:(UIButton *)sender
{
  [self.delegate progressButtonPressed:self];
}
#pragma mark - Properties

- (void)setProgress:(CGFloat)progress
{
  if (progress < _progress || progress <= 0)
  {
    self.state = MWMCircularProgressStateSpinner;
    return;
  }
  self.rootView.state = MWMCircularProgressStateProgress;
  if (self.rootView.animating)
  {
    if (progress > self.nextProgressToAnimate.floatValue)
      self.nextProgressToAnimate = @(progress);
  }
  else
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self.rootView animateFromValue:self->_progress toValue:progress];
      self->_progress = progress;
    });
  }
}

- (void)setState:(MWMCircularProgressState)state
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [self reset];
    self.rootView.state = state;
  });
}

- (MWMCircularProgressState)state
{
  return self.rootView.state;
}
@end
