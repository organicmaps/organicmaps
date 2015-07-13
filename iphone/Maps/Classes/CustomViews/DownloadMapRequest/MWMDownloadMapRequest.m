//
//  MWMDownloadMapRequest.m
//  Maps
//
//  Created by Ilya Grechuhin on 10.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Macros.h"
#import "MWMDownloadMapRequest.h"
#import "MWMDownloadMapRequestView.h"
#import "MWMCircularProgress.h"
#import "UIButton+RuntimeAttributes.h"
#import "UIColor+MapsMeColor.h"

@interface MWMDownloadMapRequest ()

@property (nonatomic) IBOutlet MWMDownloadMapRequestView * rootView;
@property (nonatomic) IBOutlet UILabel * mapTitleLabel;
@property (nonatomic) IBOutlet UIButton * downloadMapButton;
@property (nonatomic) IBOutlet UIButton * downloadRoutesButton;
@property (nonatomic) IBOutlet UILabel * undefinedLocationLabel;
@property (nonatomic) IBOutlet UIButton * selectAnotherMapButton;
@property (nonatomic) IBOutlet UIView * progressViewWrapper;

@property (nonatomic) MWMCircularProgress * progressView;

@property (copy, nonatomic) MWMDownloadMapRequestDownloadCallback downloadBlock;
@property (copy, nonatomic) MWMDownloadMapRequestSelectCallback selectBlock;

@property (copy, nonatomic) NSString * mapSize;
@property (copy, nonatomic) NSString * mapAndRouteSize;

@property (nonatomic) enum MWMDownloadMapRequestState state;

@property (nonnull, weak, nonatomic) id <MWMCircularProgressDelegate> delegate;

@end

@implementation MWMDownloadMapRequest

- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView  delegate:(nonnull id <MWMCircularProgressDelegate>)delegate
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
    [parentView addSubview:self.rootView];
    self.rootView.hidden = YES;
    self.downloadRoutesButton.selected = YES;
    self.delegate = delegate;
  }
  return self;
}

- (void)dealloc
{
  [self.rootView removeFromSuperview];
}

- (void)showForLocationWithName:(nonnull NSString *)countryName mapSize:(nonnull NSString *)mapSize mapAndRouteSize:(nonnull NSString *)mapAndRouteSize download:(nonnull MWMDownloadMapRequestDownloadCallback)download select:(nonnull MWMDownloadMapRequestSelectCallback)select
{
  [self stopDownload];
  self.mapTitleLabel.text = countryName;
  [self.downloadMapButton setTitle:[NSString stringWithFormat:@"%@ (%@)", L(@"downloader_download_map"), mapAndRouteSize] forState:UIControlStateNormal];
  [self.downloadRoutesButton setTitle:L(@"search_vehicle_routes") forState:UIControlStateNormal];

  [self.selectAnotherMapButton setTitle:L(@"search_select_other_map") forState:UIControlStateNormal];
  [self.selectAnotherMapButton setTitleColor:[UIColor primary] forState:UIControlStateNormal];
  [self.selectAnotherMapButton setTitleColor:[UIColor whiteColor] forState:UIControlStateHighlighted];
  [self.selectAnotherMapButton setBackgroundColor:[UIColor whiteColor] forState:UIControlStateNormal];
  [self.selectAnotherMapButton setBackgroundColor:[UIColor primary] forState:UIControlStateHighlighted];

  self.mapSize = mapSize;
  self.mapAndRouteSize = mapAndRouteSize;
  self.downloadBlock = download;
  self.selectBlock = select;
}

- (void)showForUnknownLocation:(nonnull MWMDownloadMapRequestSelectCallback)select
{
  self.rootView.hidden = NO;
  self.progressViewWrapper.hidden = YES;
  self.mapTitleLabel.hidden = YES;
  self.downloadMapButton.hidden = YES;
  self.downloadRoutesButton.hidden = YES;
  self.undefinedLocationLabel.hidden = NO;
  [self.selectAnotherMapButton setTitle:L(@"search_select_map") forState:UIControlStateNormal];

  [self.selectAnotherMapButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
  [self.selectAnotherMapButton setBackgroundColor:[UIColor primary] forState:UIControlStateNormal];
  [self.selectAnotherMapButton setBackgroundColor:[UIColor primaryDark] forState:UIControlStateHighlighted];

  self.selectBlock = select;
  self.state = MWMDownloadMapRequestUnknownLocation;
}

#pragma mark - Process control

- (void)startDownload
{
  self.progressView = [[MWMCircularProgress alloc] initWithParentView:self.progressViewWrapper delegate:self.delegate];
  self.rootView.hidden = NO;
  self.progressViewWrapper.hidden = NO;
  self.mapTitleLabel.hidden = NO;
  self.downloadMapButton.hidden = YES;
  self.downloadRoutesButton.hidden = YES;
  self.undefinedLocationLabel.hidden = YES;
  self.selectAnotherMapButton.hidden = YES;
  self.state = MWMDownloadMapRequestProgress;
}

- (void)stopDownload
{
  self.rootView.hidden = NO;
  self.progressViewWrapper.hidden = YES;
  self.mapTitleLabel.hidden = NO;
  self.downloadMapButton.hidden = NO;
  self.downloadRoutesButton.hidden = NO;
  self.undefinedLocationLabel.hidden = YES;
  self.selectAnotherMapButton.hidden = NO;
  self.state = MWMDownloadMapRequestLocation;
}

- (void)downloadProgress:(CGFloat)progress countryName:(nonnull NSString *)countryName
{
  self.progressView.failed = NO;
  self.progressView.progress = progress;
  self.mapTitleLabel.text = countryName;
}

- (void)setDownloadFailed
{
  self.progressView.failed = YES;
}

#pragma mark - Actions

- (IBAction)downloadMapTouchUpInside:(nonnull UIButton *)sender
{
  self.downloadBlock(self.downloadRoutesButton.selected);
}

- (IBAction)downloadRoutesTouchUpInside:(nonnull UIButton *)sender
{
  sender.selected = !sender.selected;
  [self.downloadMapButton setTitle:[NSString stringWithFormat:@"%@ (%@)", L(@"downloader_download_map"), sender.selected ? self.mapAndRouteSize : self.mapSize] forState:UIControlStateNormal];
}

- (IBAction)selectMapTouchUpInside:(nonnull UIButton *)sender
{
  self.selectBlock();
}

#pragma mark - Properties

- (void)setState:(enum MWMDownloadMapRequestState)state
{
  _state = state;
  [self.rootView layoutSubviews];
}

@end
