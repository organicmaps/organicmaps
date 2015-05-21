//
//  MWMPlacePageView.m
//  Maps
//
//  Created by v.mikhaylenko on 18.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePage.h"
#import "MWMBasePlacePageView.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePageActionBar.h"

static NSString * const kPlacePageNibIdentifier = @"PlacePageView";

@interface MWMPlacePage ()

@property (weak, nonatomic, readwrite) MWMPlacePageViewManager * manager;

@end

@implementation MWMPlacePage

- (instancetype)initWithManager:(MWMPlacePageViewManager *)manager
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:kPlacePageNibIdentifier owner:self options:nil];
    self.manager = manager;
    [self configure];
  }
  return self;
}

- (void)configure
{
  [self.basePlacePageView configureWithEntity:self.manager.entity];
}

- (void)show
{
  [self doesNotRecognizeSelector:_cmd];
}

- (void)dismiss
{
  [self.extendedPlacePageView removeFromSuperview];
}

- (void)addBookmark
{
  [self doesNotRecognizeSelector:_cmd];
}

- (IBAction)didTap:(UITapGestureRecognizer *)sender { }

- (IBAction)didPan:(UIPanGestureRecognizer *)sender { }

@end
