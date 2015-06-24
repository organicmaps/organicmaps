//
//  MWMLocationButtonView.m
//  Maps
//
//  Created by Ilya Grechuhin on 14.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMLocationButtonView.h"
#import "MWMMapViewControlsCommon.h"
#import "UIKitCategories.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

@interface MWMLocationButtonView()

@property (nonatomic) CGRect defaultBounds;

@end

@implementation MWMLocationButtonView

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
  {
    self.defaultBounds = self.bounds;
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.locationState = location::State::UnknownPosition;
  }
  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.bounds = self.defaultBounds;
  [self layoutXPosition:self.hidden];
  self.maxY = self.superview.height - 2.0 * kViewControlsOffsetToBounds;
}

- (void)layoutXPosition:(BOOL)hidden
{
  if (hidden)
    self.maxX = 0.0;
  else
    self.minX = 2.0 * kViewControlsOffsetToBounds;
}

- (void)setImageNamed:(location::State::Mode)state
{
  if (state == location::State::PendingPosition)
    [self setPendingPositionAnimation];
  else
  {
    static NSDictionary const * const stateMap = @{@(location::State::UnknownPosition).stringValue : @"btn_white_unknow_position",
                                                   @(location::State::NotFollow).stringValue : @"btn_white_target_off_1",
                                                   @(location::State::Follow).stringValue : @"btn_white_target_on",
                                                   @(location::State::RotateAndFollow).stringValue : @"btn_white_direction"};
    [self.imageView stopAnimating];
    NSString * const imageName = stateMap[@(state).stringValue];
    self.highlighted = NO;
    [self setImage:[UIImage imageNamed:imageName] forState:UIControlStateNormal];
    [self setImage:[UIImage imageNamed:[imageName stringByAppendingString:@"_pressed"]] forState:UIControlStateHighlighted];
  }
}

- (void)setAnimation:(NSArray *)animationImages once:(BOOL)once
{
  UIImageView const * const animationIV = self.imageView;
  animationIV.animationImages = animationImages;
  animationIV.animationDuration = framesDuration(animationImages.count);
  animationIV.animationRepeatCount = once ? 1 : 0;
  [animationIV startAnimating];
}

- (void)setPendingPositionAnimation
{
  NSString const * const imageName = @"btn_white_loading_";
  [self setImage:[UIImage imageNamed:[imageName stringByAppendingString:@"1"]] forState:UIControlStateNormal];
  static NSUInteger const animationImagesCount = 18;
  NSMutableArray * const animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
    animationImages[i] = [UIImage imageNamed:[NSString stringWithFormat:@"%@%@", imageName, @(i+1)]];
  [self setAnimation:animationImages once:NO];
}

- (void)changeButtonFromState:(location::State::Mode)beginState toState:(location::State::Mode)endState
{
  NSAssert1(beginState != endState, @"MWMLocationButtonView::(changeButtonFromState:toState:) states must be different! state:%@", @(beginState));
  [self setImageNamed:endState];
  static NSDictionary const * const stateMap = @{@(location::State::UnknownPosition).stringValue : @"noposition",
                                                 @(location::State::PendingPosition).stringValue : @"pending",
                                                 @(location::State::NotFollow).stringValue : @"notfollow",
                                                 @(location::State::Follow).stringValue : @"follow",
                                                 @(location::State::RotateAndFollow).stringValue : @"followandrotate"};
  NSString const * const changeAnimation = [NSString stringWithFormat:@"%@_to_%@_", stateMap[@(beginState).stringValue], stateMap[@(endState).stringValue]];
  static NSUInteger const animationImagesCount = 6;
  NSMutableArray * const animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
  {
    NSString * imageName = [NSString stringWithFormat:@"%@%@", changeAnimation, @(i+1)];
    UIImage * image = [UIImage imageNamed:imageName];
    NSAssert1(image, @"MWMLocationButtonView::(changeButtonFromState:toState:) image is nil! imageName: %@", imageName);
    if (image)
    {
      animationImages[i] = image;
    }
    else
    {
      // TODO(grechuhin): Temporary, check Alohalytics logs and clean implementation.
      [Alohalytics logEvent:@"MWMLocationButtonViewChangeButtonFromStateToStateInvalidImageName" withValue:imageName];
      [self changeStateFinish];
      return;
    }
  }
  [self setAnimation:animationImages once:YES];
  [self changeStateFinish];
}

- (void)changeStateFinish
{
  dispatch_async(dispatch_get_main_queue(), ^
  {
    if (self.imageView.isAnimating)
      [self changeStateFinish];
    else
      [self setImageNamed:self.locationState];
  });
}

#pragma mark - Properties

- (void)setLocationState:(location::State::Mode)locationState
{
  if (_locationState == locationState)
    [self setImageNamed:locationState];
  else
  {
    [self changeButtonFromState:_locationState toState:locationState];
    _locationState = locationState;
  }
}

- (void)setHidden:(BOOL)hidden
{
  if (!hidden)
    super.hidden = NO;
  [self layoutXPosition:!hidden];
  [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount) animations:^
  {
    [self layoutXPosition:hidden];
  }
  completion:^(BOOL finished)
  {
    if (hidden)
      super.hidden = YES;
  }];
}

@end
