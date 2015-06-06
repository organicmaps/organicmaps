//
//  MWMLocationButton.m
//  Maps
//
//  Created by Ilya Grechuhin on 14.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMLocationButton.h"
#import "MWMLocationButtonView.h"

#include "Framework.h"

static NSString * const kMWMLocationButtonViewNibName = @"MWMLocationButton";

@interface MWMLocationButton()

@property (nonatomic) IBOutlet MWMLocationButtonView * button;

@end

@implementation MWMLocationButton

- (instancetype)initWithParentView:(UIView *)view
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:kMWMLocationButtonViewNibName owner:self options:nil];
    [view addSubview:self.button];
    [self configLocationListener];
  }
  return self;
}

- (void)configLocationListener
{
  typedef void (*LocationStateModeFnT)(id, SEL, location::State::Mode);
  SEL locationStateModeSelector = @selector(onLocationStateModeChanged:);
  LocationStateModeFnT locationStateModeFn = (LocationStateModeFnT)[self methodForSelector:locationStateModeSelector];
  
  GetFramework().GetLocationState()->AddStateModeListener(bind(locationStateModeFn, self, locationStateModeSelector, _1));
}

- (void)onLocationStateModeChanged:(location::State::Mode)state
{
  self.button.locationState = state;
}

- (IBAction)buttonPressed:(id)sender
{
  GetFramework().GetLocationState()->SwitchToNextMode();
}

#pragma mark - Properties

- (BOOL)hidden
{
  return self.button.hidden;
}

- (void)setHidden:(BOOL)hidden
{
  self.button.hidden = hidden;
}

@end
