#import "MWMZoomButtons.h"
#import "MWMZoomButtonsView.h"
#import "Statistics.h"

#include "Framework.h"
#include "indexer/scales.hpp"
#include "platform/settings.hpp"

static NSString * const kMWMZoomButtonsViewNibName = @"MWMZoomButtonsView";

@interface MWMZoomButtons ()

@property(nonatomic) IBOutlet MWMZoomButtonsView * zoomView;
@property(weak, nonatomic) IBOutlet UIButton * zoomInButton;
@property(weak, nonatomic) IBOutlet UIButton * zoomOutButton;

@property(nonatomic) BOOL zoomSwipeEnabled;
@property(nonatomic, readonly) BOOL isZoomEnabled;

@end

@implementation MWMZoomButtons

- (instancetype)initWithParentView:(UIView *)view
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:kMWMZoomButtonsViewNibName owner:self options:nil];
    [view addSubview:self.zoomView];
    [self.zoomView layoutIfNeeded];
    self.zoomView.topBound = 0.0;
    self.zoomView.bottomBound = view.height;
    self.zoomSwipeEnabled = NO;
  }
  return self;
}

- (void)setTopBound:(CGFloat)bound
{
  self.zoomView.topBound = bound;
}

- (void)setBottomBound:(CGFloat)bound
{
  self.zoomView.bottomBound = bound;
}

- (void)zoomIn
{
  [Statistics logEvent:kStatEventName(kStatZoom, kStatIn)];
  GetFramework().Scale(Framework::SCALE_MAG, true);
}

- (void)zoomOut
{
  [Statistics logEvent:kStatEventName(kStatZoom, kStatOut)];
  GetFramework().Scale(Framework::SCALE_MIN, true);
}

- (void)mwm_refreshUI
{
  [self.zoomView mwm_refreshUI];
}

#pragma mark - Actions

- (IBAction)zoomTouchDown:(UIButton *)sender
{
  self.zoomSwipeEnabled = YES;
}

- (IBAction)zoomTouchUpInside:(UIButton *)sender
{
  self.zoomSwipeEnabled = NO;
  if ([sender isEqual:self.zoomInButton])
    [self zoomIn];
  else
    [self zoomOut];
}

- (IBAction)zoomTouchUpOutside:(UIButton *)sender
{
  self.zoomSwipeEnabled = NO;
}

- (IBAction)zoomSwipe:(UIPanGestureRecognizer *)sender
{
  if (!self.zoomSwipeEnabled)
    return;
  UIView * const superview = self.zoomView.superview;
  CGFloat const translation = -[sender translationInView:superview].y / superview.bounds.size.height;

  CGFloat const scaleFactor = exp(translation);
  GetFramework().Scale(scaleFactor, false);
}

#pragma mark - Properties

- (BOOL)isZoomEnabled
{
  bool zoomButtonsEnabled = true;
  (void)settings::Get("ZoomButtonsEnabled", zoomButtonsEnabled);
  return zoomButtonsEnabled;
}

- (BOOL)hidden
{
  return self.isZoomEnabled ? self.zoomView.hidden : YES;
}

- (void)setHidden:(BOOL)hidden
{
  if (self.isZoomEnabled)
    [self.zoomView setHidden:hidden animated:YES];
  else
    self.zoomView.hidden = YES;
}

@end
