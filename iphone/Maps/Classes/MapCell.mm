#import "Common.h"
#import "MapCell.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"
#import "UIImageView+Coloring.h"

@interface MapCell () <ProgressViewDelegate>

@property (nonatomic) UILabel * titleLabel;
@property (nonatomic) UILabel * subtitleLabel;
@property (nonatomic) UILabel * statusLabel;
@property (nonatomic) UILabel * sizeLabel;
@property (nonatomic) ProgressView * progressView;
@property (nonatomic) UIImageView * arrowView;
@property (nonatomic) BadgeView * badgeView;
@property (nonatomic) UIImageView * routingImageView;

@property (nonatomic) UIView *separatorTop;
@property (nonatomic) UIView *separator;
@property (nonatomic) UIView *separatorBottom;

@property (nonatomic, readonly) BOOL progressMode;

@end

@implementation MapCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  NSArray * subviews = @[self.titleLabel, self.subtitleLabel, self.statusLabel, self.sizeLabel, self.progressView, self.arrowView, self.badgeView, self.routingImageView, self.separator, self.separatorTop, self.separatorBottom];
  for (UIView * subview in subviews)
    [self.contentView addSubview:subview];
  self.backgroundColor = [UIColor white];
  self.titleLabel.textColor = [UIColor blackPrimaryText];
  self.subtitleLabel.textColor = self.statusLabel.textColor = self.sizeLabel.textColor = [UIColor blackHintText];

  return self;
}

- (void)setStatus:(TStatus)status options:(MapOptions)options animated:(BOOL)animated
{
  self.status = status;
  self.options = options;

  self.progressView.failedMode = NO;

  if (options == MapOptions::Map)
    self.routingImageView.mwm_name = @"ic_routing_get";
  else
    self.routingImageView.mwm_name = @"ic_routing_ok";

  switch (status)
  {
    case TStatus::ENotDownloaded:
    case TStatus::EOnDiskOutOfDate:
      if (status == TStatus::ENotDownloaded)
        self.statusLabel.text = L(@"download").uppercaseString;
      else
       self.statusLabel.text = L(@"downloader_status_outdated").uppercaseString;

      self.statusLabel.textColor = [UIColor linkBlue];
      [self setProgressMode:NO withAnimatedLayout:animated];
      break;

    case TStatus::EInQueue:
    case TStatus::EDownloading:
      self.statusLabel.textColor = [UIColor blackHintText];
      [self setDownloadProgress:self.downloadProgress animated:animated];
      if (status == TStatus::EInQueue)
        self.statusLabel.text = L(@"downloader_queued").uppercaseString;
      break;

    case TStatus::EOnDisk:
    {
      self.statusLabel.text = L(@"downloader_downloaded").uppercaseString;
      self.statusLabel.textColor = [UIColor blackHintText];
      if (animated)
      {
        [self alignSubviews];
        [self performAfterDelay:0.3 block:^{
          [self setProgressMode:NO withAnimatedLayout:YES];
        }];
      }
      else
      {
        [self setProgressMode:NO withAnimatedLayout:NO];
      }
      break;
    }

    case TStatus::EOutOfMemFailed:
    case TStatus::EDownloadFailed:
      self.progressView.failedMode = YES;
      self.statusLabel.text = L(@"downloader_retry").uppercaseString;
      self.statusLabel.textColor = [UIColor red];
      [self setProgressMode:YES withAnimatedLayout:animated];
      break;

    case TStatus::EUnknown:
      break;
  }
}

- (void)setDownloadProgress:(double)downloadProgress animated:(BOOL)animated
{
  self.downloadProgress = downloadProgress;
  self.statusLabel.text = [NSString stringWithFormat:@"%li%%", long(downloadProgress * 100)];
  [self.progressView setProgress:downloadProgress animated:animated];
  if (!self.progressMode)
    [self setProgressMode:YES withAnimatedLayout:animated];
}

- (void)setProgressMode:(BOOL)progressMode withAnimatedLayout:(BOOL)withLayout
{
  _progressMode = progressMode;

  self.progressView.progress = self.downloadProgress;
  if (withLayout)
  {
    if (progressMode)
      self.progressView.hidden = NO;
    [UIView animateWithDuration:0.5 delay:0 damping:0.9 initialVelocity:0 options:UIViewAnimationOptionCurveEaseIn animations:^{
      [self alignProgressView];
      [self alignSubviews];
    } completion:^(BOOL finished) {
      if (!progressMode)
        self.progressView.hidden = YES;
    }];
  }
  else
  {
    [self alignProgressView];
    [self alignSubviews];
  }
}

- (void)alignProgressView
{
  self.progressView.minX = self.progressMode ? self.contentView.width - [self rightOffset] + 2 : self.contentView.width;
}

- (void)alignSubviews
{
  self.progressView.hidden = self.parentMode || !self.progressMode;
  self.progressView.midY = self.contentView.height / 2;

  self.arrowView.center = CGPointMake(self.contentView.width - [self minimumRightOffset] - 4, self.contentView.height / 2);
  self.arrowView.hidden = !self.parentMode;

  [self.statusLabel sizeToIntegralFit];
  self.statusLabel.width = MAX(self.statusLabel.width, 60);
  [self.sizeLabel sizeToIntegralFit];
  self.statusLabel.frame = CGRectMake(self.contentView.width - [self rightOffset] - self.statusLabel.width, 14, self.statusLabel.width, 16);
  self.statusLabel.hidden = self.parentMode;

  CGFloat const sizeLabelMinY = self.statusLabel.maxY;
  self.sizeLabel.frame = CGRectMake(self.contentView.width - [self rightOffset] - self.sizeLabel.width, sizeLabelMinY, self.sizeLabel.width, 16);
  self.sizeLabel.textColor = [UIColor blackHintText];
  self.sizeLabel.hidden = self.parentMode;

  CGFloat const rightLabelsMaxWidth = self.parentMode ? 10 : MAX(self.statusLabel.width, self.sizeLabel.width);
  CGFloat const leftLabelsWidth = self.contentView.width - [self leftOffset] - [self betweenSpace] - rightLabelsMaxWidth - [self rightOffset];

  CGFloat const titleLabelWidth = [self.titleLabel.text sizeWithDrawSize:CGSizeMake(1000, 20) font:self.titleLabel.font].width;
  self.titleLabel.frame = CGRectMake([self leftOffset], self.subtitleLabel.text == nil ? 19 : 10, MIN(titleLabelWidth, leftLabelsWidth), 20);
  self.subtitleLabel.frame = CGRectMake([self leftOffset], self.titleLabel.maxY + 1, leftLabelsWidth, 18);
  self.subtitleLabel.hidden = self.subtitleLabel.text == nil;

  self.routingImageView.center = CGPointMake(self.contentView.width - 25, self.contentView.height / 2 - 1);
  self.routingImageView.alpha = [self shouldShowRoutingView];

  self.separatorTop.frame = CGRectMake(0, 0, self.contentView.width, PIXEL);
  self.separatorTop.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;

  self.separatorBottom.frame = CGRectMake(0, self.contentView.height - PIXEL, self.contentView.width, PIXEL);
  self.separatorBottom.autoresizingMask = UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleTopMargin;
}

- (void)prepareForReuse
{
  self.separatorTop.hidden = YES;
  self.separatorBottom.hidden = YES;
  self.separator.hidden = NO;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  [self alignSubviews];
  if (!self.parentMode)
  {
    [self alignProgressView];
    [self setStatus:self.status options:self.options animated:NO];
  }
  self.badgeView.minX = self.titleLabel.maxX + 3;
  self.badgeView.minY = self.titleLabel.minY - 5;

  self.separator.minX = self.titleLabel.minX;
  self.separator.size = CGSizeMake(self.contentView.width - 2 * self.separator.minX, PIXEL);
  self.separator.maxY = self.contentView.height;
}


- (BOOL)shouldShowRoutingView
{
  return !self.progressMode && !self.parentMode && self.status != TStatus::ENotDownloaded;
}

- (CGFloat)leftOffset
{
  return 12;
}

- (CGFloat)betweenSpace
{
  return 10;
}

- (CGFloat)rightOffset
{
  return self.progressMode || [self shouldShowRoutingView] ? 50 : [self minimumRightOffset];
}

- (CGFloat)minimumRightOffset
{
  return 12;
}

+ (CGFloat)cellHeight
{
  return 59;
}

- (void)rightTap:(id)sender
{
  [self.delegate mapCellDidStartDownloading:self];
}

- (void)progressViewDidStart:(ProgressView *)progress
{
  [self.delegate mapCellDidStartDownloading:self];
}

- (void)progressViewDidCancel:(ProgressView *)progress
{
  [self.delegate mapCellDidCancelDownloading:self];
}

- (UIImageView *)arrowView
{
  if (!_arrowView)
    _arrowView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"AccessoryView"]];
  return _arrowView;
}

- (ProgressView *)progressView
{
  if (!_progressView)
  {
    _progressView = [[ProgressView alloc] init];
    _progressView.delegate = self;
  }
  return _progressView;
}

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.textColor = [UIColor blackPrimaryText];
    _titleLabel.font = [UIFont regular17];
  }
  return _titleLabel;
}

- (UILabel *)subtitleLabel
{
  if (!_subtitleLabel)
  {
    _subtitleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _subtitleLabel.backgroundColor = [UIColor clearColor];
    _subtitleLabel.textColor = [UIColor blackSecondaryText];
    _subtitleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:13];
  }
  return _subtitleLabel;
}

- (UILabel *)statusLabel
{
  if (!_statusLabel)
  {
    _statusLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _statusLabel.backgroundColor = [UIColor clearColor];
    _statusLabel.textAlignment = NSTextAlignmentRight;
    _statusLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:13];
  }
  return _statusLabel;
}

- (UILabel *)sizeLabel
{
  if (!_sizeLabel)
  {
    _sizeLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _sizeLabel.backgroundColor = [UIColor clearColor];
    _sizeLabel.textAlignment = NSTextAlignmentRight;
    _sizeLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:13];
  }
  return _sizeLabel;
}

- (UIImageView *)routingImageView
{
  if (!_routingImageView)
  {
    _routingImageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"ic_routing_get_light"]];
    _routingImageView.mwm_name = @"ic_routing_get";
  }
  return _routingImageView;
}

- (BadgeView *)badgeView
{
  if (!_badgeView)
    _badgeView = [[BadgeView alloc] init];
  return _badgeView;
}

- (UIView *)separatorTop
{
  if (!_separatorTop)
  {
    _separatorTop = [[UIView alloc] initWithFrame:CGRectZero];
    _separatorTop.backgroundColor = [UIColor blackDividers];
  }
  return _separatorTop;
}

- (UIView *)separator
{
  if (!_separator)
  {
    _separator = [[UIView alloc] initWithFrame:CGRectZero];
    _separator.backgroundColor = [UIColor blackDividers];
  }
  return _separator;
}

- (UIView *)separatorBottom
{
  if (!_separatorBottom)
  {
    _separatorBottom = [[UIView alloc] initWithFrame:CGRectZero];
    _separatorBottom.backgroundColor = [UIColor blackDividers];
  }
  return _separatorBottom;
}

@end
