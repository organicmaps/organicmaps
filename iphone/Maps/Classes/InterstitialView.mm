
#import "InterstitialView.h"
#import "UIKitCategories.h"
#import "NavigationController.h"

@interface InterstitialView ()

@property (nonatomic) UIView * fadeView;
@property (nonatomic) UIView * contentView;

@property (nonatomic, weak) id <InterstitialViewDelegate> delegate;
@property (nonatomic) NSString * inAppMessageName;
@property (nonatomic) NSString * imageType;
@property (nonatomic) UIImageView * portraitImageView;
@property (nonatomic) UIImageView * landscapeImageView;

@end

@implementation InterstitialView

- (instancetype)initWithImages:(NSArray *)images inAppMessageName:(NSString *)messageName imageType:(NSString *)imageType delegate:(id<InterstitialViewDelegate>)delegate
{
  self = [super initWithFrame:[self viewWindow].bounds];

  if (IPAD && [images count] != 2)
    return self;
  if (!IPAD && [images count] != 1)
    return self;

  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

  UIView * fadeView = [[UIView alloc] initWithFrame:self.bounds];
  fadeView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.8];
  fadeView.alpha = 0;
  fadeView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [self addSubview:fadeView];
  self.fadeView = fadeView;

  UIView * contentView = [[UIView alloc] initWithFrame:self.bounds];
  contentView.minY = self.height;
  contentView.midX = self.width / 2;
  contentView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [self addSubview:contentView];
  self.contentView = contentView;

  UIImage * portraitImage = images[0];
  UIImageView * portraitImageView = [[UIImageView alloc] initWithImage:portraitImage];
  portraitImageView.userInteractionEnabled = YES;
  UITapGestureRecognizer * tap1 = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
  [portraitImageView addGestureRecognizer:tap1];
  portraitImageView.center = CGPointMake(self.contentView.width / 2, self.contentView.height / 2);
  [self.contentView addSubview:portraitImageView];
  self.portraitImageView = portraitImageView;

  if (IPAD)
  {
    UIImage * landscapeImage = images[1];
    UIImageView * landscapeImageView = [[UIImageView alloc] initWithImage:landscapeImage];
    landscapeImageView.userInteractionEnabled = YES;
    UITapGestureRecognizer * tap2 = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
    [landscapeImageView addGestureRecognizer:tap2];
    [self.contentView addSubview:landscapeImageView];
    self.landscapeImageView = landscapeImageView;
  }

  self.inAppMessageName = messageName;
  self.imageType = imageType;
  self.delegate = delegate;

  UIButton * closeButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, self.width, 70)];
  closeButton.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;
  [closeButton addTarget:self action:@selector(closeButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  [self.contentView addSubview:closeButton];

  [self updateAnimated:NO];

  return self;
}

- (void)layoutSubviews
{
  [self updateAnimated:YES];
}

- (void)updateAnimated:(BOOL)animated
{
  if (IPAD)
  {
    BOOL portrait = self.width < self.height;
    UIImageView * currentImageView = portrait ? self.portraitImageView : self.landscapeImageView;
    UIImageView * prevImageView = portrait ? self.landscapeImageView : self.portraitImageView;

    CGFloat scale = MIN(1, MIN(self.height / currentImageView.height, self.width / currentImageView.width));
    [UIView animateWithDuration:(animated ? 0.3 : 0) delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
      currentImageView.transform = CGAffineTransformMakeScale(scale, scale);
      currentImageView.center = CGPointMake(self.contentView.width / 2, self.contentView.height / 2);
      prevImageView.alpha = 0;
      currentImageView.alpha = 1;
    } completion:^(BOOL finished) {}];
  }
}

- (void)tap:(id)sender
{
  [self closeWithResult:InterstitialViewResultClicked];
}

- (void)closeButtonPressed:(id)sender
{
  [self closeWithResult:InterstitialViewResultDismissed];
}

- (UIView *)viewWindow
{
  UIWindow * window = [[UIApplication sharedApplication].windows firstObject];
  // we do this because on iPhone messages are only in portrait mode
  return IPAD ? [window.subviews firstObject] : window;
}

- (void)show
{
  UIView * view = [self viewWindow];
  if (!IPAD)
    [self blockRotation:YES];

  for (UIView * subview in view.subviews)
  {
    if ([subview isKindOfClass:[self class]])
      [subview removeFromSuperview];
  }
  [view addSubview:self];
  [view endEditing:YES];
  [self.delegate interstitialViewWillOpen:self];
  if ([UIView respondsToSelector:@selector(animateWithDuration:delay:usingSpringWithDamping:initialSpringVelocity:options:animations:completion:)])
  {
    [UIView animateWithDuration:0.5 delay:0 usingSpringWithDamping:1 initialSpringVelocity:0.2 options:UIViewAnimationOptionCurveEaseInOut animations:^{
      self.fadeView.alpha = 1;
      self.contentView.midY = self.height / 2;
    } completion:^(BOOL finished){}];
  }
  else
  {
    [UIView animateWithDuration:0.4 delay:0 options:UIViewAnimationOptionCurveEaseOut animations:^{
      self.fadeView.alpha = 1;
      self.contentView.midY = self.height / 2;
    } completion:^(BOOL finished){}];
  }
}

- (void)closeWithResult:(InterstitialViewResult)result
{
  [UIView animateWithDuration:0.3 delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    self.fadeView.alpha = 0;
    self.contentView.minY = self.height;
  } completion:^(BOOL finished){
    [self.delegate interstitialView:self didCloseWithResult:result];
    [self removeFromSuperview];
    [self blockRotation:NO];
  }];
}

- (void)blockRotation:(BOOL)block
{
  UIWindow * window = [[UIApplication sharedApplication].windows firstObject];
  NavigationController * vc = (NavigationController *)window.rootViewController;
  vc.autorotate = !block;
}

- (void)dealloc
{
  [self blockRotation:NO];
}

@end
