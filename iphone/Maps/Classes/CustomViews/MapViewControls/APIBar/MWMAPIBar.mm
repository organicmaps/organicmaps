//
//  MWMAPIBar.m
//  Maps
//
//  Created by Ilya Grechuhin on 27.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAPIBar.h"
#import "MWMAPIBarView.h"
#import "SearchView.h"
#import "UIKitCategories.h"

#include "Framework.h"

@interface MWMAPIBar ()

@property (nonatomic) IBOutlet MWMAPIBarView * rootView;
@property (nonatomic) IBOutlet UILabel * titleLabel;

@property (weak, nonatomic) UIViewController<MWMAPIBarProtocol> * delegate;
@property (nonatomic, setter = setVisible:) BOOL isVisible;

@end

@implementation MWMAPIBar

- (instancetype)initWithDelegate:(nonnull UIViewController<MWMAPIBarProtocol> *)delegate
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:@"MWMAPIBarView" owner:self options:nil];
    self.delegate = delegate;
  }
  return self;
}

- (void)show
{
  self.titleLabel.text = [NSString stringWithUTF8String:GetFramework().GetApiDataHolder().GetAppTitle().c_str()];
  [self.delegate.view insertSubview:self.rootView belowSubview:self.delegate.searchView];
  self.rootView.width = self.delegate.view.width;
  self.rootView.maxY = 0.0;
  [UIView animateWithDuration:0.2 animations:^
  {
    self.rootView.targetY = 0.0;
    self.isVisible = YES;
  }];
}

- (void)hideAnimated:(BOOL)animated
{
  [UIView animateWithDuration:animated ? 0.2 : 0.0 animations:^
  {
    self.rootView.targetY = -self.rootView.height;
    self.isVisible = NO;
  }
  completion:^(BOOL finished)
  {
    [self.rootView removeFromSuperview];
  }];
}

#pragma mark - Actions

- (IBAction)backButtonTouchUpInside:(UIButton *)sender
{
  NSURL * url = [NSURL URLWithString:[NSString stringWithUTF8String:GetFramework().GetApiDataHolder().GetGlobalBackUrl().c_str()]];
  [[UIApplication sharedApplication] openURL:url];
  [self hideAnimated:NO];
}

- (IBAction)clearButtonTouchUpInside:(UIButton *)sender
{
  [self hideAnimated:YES];
  auto & f = GetFramework();
  auto & bm = f.GetBalloonManager();
  bm.RemovePin();
  bm.Dismiss();
  f.GetBookmarkManager().UserMarksClear(UserMarkContainer::API_MARK);
  f.Invalidate();
}

#pragma mark - Properties

- (void)setVisible:(BOOL)visible
{
  _isVisible = visible;
  [self.delegate apiBarBecameVisible:visible];
}

- (CGRect)frame
{
  return self.rootView.frame;
}

@end
