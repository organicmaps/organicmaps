//
//  MWMSearchDownloadMapRequest.m
//  Maps
//
//  Created by Ilya Grechuhin on 10.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMSearchDownloadMapRequest.h"
#import "MWMSearchDownloadMapRequestView.h"

@interface MWMSearchDownloadMapRequest ()

@property (nonatomic) IBOutlet MWMSearchDownloadMapRequestView * rootView;
@property (nonatomic) IBOutlet UIView * downloadRequestHolder;

@property (nonatomic) MWMDownloadMapRequest * downloadRequest;
@property (strong, nonatomic) IBOutlet UIButton * dimButton;

@end

@implementation MWMSearchDownloadMapRequest

- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView delegate:(nonnull id <MWMCircularProgressDelegate>)delegate
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
    [parentView addSubview:self.rootView];
    self.downloadRequest = [[MWMDownloadMapRequest alloc] initWithParentView:self.downloadRequestHolder delegate:delegate];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillhide:) name:UIKeyboardWillHideNotification object:nil];
  }
  return self;
}

- (void)showForLocationWithName:(nonnull NSString *)locationName mapSize:(nonnull NSString *)mapSize mapAndRouteSize:(nonnull NSString *)mapAndRouteSize download:(nonnull MWMDownloadMapRequestDownloadCallback)download select:(nonnull MWMDownloadMapRequestSelectCallback)select;
{
  [self.downloadRequest showForLocationWithName:locationName mapSize:mapSize mapAndRouteSize:mapAndRouteSize download:download select:select];
}

- (void)showForUnknownLocation:(nonnull MWMDownloadMapRequestSelectCallback)select
{
  [self.downloadRequest showForUnknownLocation:select];
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [self.rootView removeFromSuperview];
}

- (void)keyboardWillShow:(nonnull NSNotification *)aNotification
{
  UIButton * dim = self.dimButton;
  dim.hidden = NO;
  dim.alpha = 0.0;
  NSNumber * duration = aNotification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  [UIView animateWithDuration:duration.floatValue animations:^
  {
    dim.alpha = 1.0;
  }];
}

- (void)keyboardWillhide:(nonnull NSNotification *)aNotification
{
  UIButton * dim = self.dimButton;
  dim.alpha = 1.0;
  NSNumber * duration = aNotification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  [UIView animateWithDuration:duration.floatValue animations:^
  {
    dim.alpha = 0.0;
  }
  completion:^(BOOL finished)
  {
    dim.hidden = YES;
  }];
}

#pragma mark - Process control

- (void)startDownload
{
  self.rootView.hintHidden = YES;
  [self.downloadRequest startDownload];
}

- (void)stopDownload
{
  self.rootView.hintHidden = NO;
  [self.downloadRequest stopDownload];
}

- (void)downloadProgress:(CGFloat)progress countryName:(nonnull NSString *)countryName
{
  [self.downloadRequest downloadProgress:progress countryName:countryName];
}

- (void)setDownloadFailed
{
  [self.downloadRequest setDownloadFailed];
}

#pragma mark - Actions

- (IBAction)dimTouchUpInside:(nonnull UIButton *)sender
{
  [[UIApplication sharedApplication].keyWindow endEditing:YES];
}

@end
