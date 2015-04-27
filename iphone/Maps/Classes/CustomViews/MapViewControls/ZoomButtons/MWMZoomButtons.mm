//
//  MWMZoomButtons.m
//  Maps
//
//  Created by Ilya Grechuhin on 12.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMZoomButtons.h"
#import "MWMZoomButtonsView.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"
#include "platform/settings.hpp"
#include "indexer/scales.hpp"

static NSString * const kMWMZoomButtonsViewNibName = @"MWMZoomButtonsView";

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMZoomButtons()

@property (nonatomic) IBOutlet MWMZoomButtonsView * zoomView;
@property (weak, nonatomic) IBOutlet UIButton * zoomInButton;
@property (weak, nonatomic) IBOutlet UIButton * zoomOutButton;

@property (nonatomic) BOOL zoomSwipeEnabled;

@end

@implementation MWMZoomButtons

- (instancetype)initWithParentView:(UIView *)view
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:kMWMZoomButtonsViewNibName owner:self options:nil];
    [view addSubview:self.zoomView];
    [self resetVisibility];
    self.zoomSwipeEnabled = NO;
  }
  return self;
}

- (void)resetVisibility
{
  bool zoomButtonsEnabled;
  if (!Settings::Get("ZoomButtonsEnabled", zoomButtonsEnabled))
    zoomButtonsEnabled = false;
  self.zoomView.hidden = !zoomButtonsEnabled;
}

- (void)zoom:(CGFloat)scale
{
  GetFramework().Scale(scale);
}

- (void)zoomIn
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"+"];
  [self zoom:2.0];
}

- (void)zoomOut
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"-"];
  [self zoom:0.5];
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

  CGFloat const scale = pow(2, translation);
  [self zoom:scale];
}

#pragma mark - Properties

- (BOOL)hidden
{
  return self.zoomView.hidden;
}

- (void)setHidden:(BOOL)hidden
{
  self.zoomView.hidden = hidden;
}

@end
