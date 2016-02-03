#import "MWMDownloadMapRequestView.h"
#import "UIButton+RuntimeAttributes.h"
#import "UIColor+MapsMeColor.h"

@interface MWMDownloadMapRequestView ()

@property (weak, nonatomic) IBOutlet UILabel * mapTitleLabel;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * verticalFreeSpace;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * bottomSpacing;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * unknownPositionLabelBottomOffset;

@property (weak, nonatomic) IBOutlet UIButton * downloadMapButton;
@property (weak, nonatomic) IBOutlet UIButton * downloadRoutesButton;
@property (weak, nonatomic) IBOutlet UILabel * undefinedLocationLabel;
@property (weak, nonatomic) IBOutlet UIButton * selectAnotherMapButton;
@property (weak, nonatomic) IBOutlet UIView * progressViewWrapper;

@end

@implementation MWMDownloadMapRequestView

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  return self;
}

- (void)layoutSubviews
{
  UIView * superview = self.superview;
  self.frame = superview.bounds;
  BOOL const isLandscape = superview.height > superview.width;
  if (IPAD || isLandscape)
  {
    self.verticalFreeSpace.constant = 44.0;
    self.bottomSpacing.constant = 24.0;
    self.unknownPositionLabelBottomOffset.constant = 22.0;
  }
  else
  {
    self.verticalFreeSpace.constant = 20.0;
    self.bottomSpacing.constant = 8.0;
    self.unknownPositionLabelBottomOffset.constant = 18.0;
    CGFloat const iPhone6LandscapeHeight = 375.0;
    if (self.width < iPhone6LandscapeHeight)
    {
      self.mapTitleLabel.lineBreakMode = NSLineBreakByTruncatingTail;
      self.mapTitleLabel.numberOfLines = 1;
    }
    else
    {
      self.mapTitleLabel.lineBreakMode = NSLineBreakByWordWrapping;
      self.mapTitleLabel.numberOfLines = 0;
    }
  }
  [super layoutSubviews];
}

#pragma mark - Properties

- (void)setState:(enum MWMDownloadMapRequestState)state
{
  _state = state;
  switch (state)
  {
    case MWMDownloadMapRequestStateDownload:
      self.progressViewWrapper.hidden = NO;
      self.mapTitleLabel.hidden = NO;
      self.downloadMapButton.hidden = YES;
      self.downloadRoutesButton.hidden = YES;
      self.undefinedLocationLabel.hidden = YES;
      self.selectAnotherMapButton.hidden = YES;
      break;
    case MWMDownloadMapRequestStateRequestLocation:
      self.progressViewWrapper.hidden = YES;
      self.mapTitleLabel.hidden = NO;
      self.downloadMapButton.hidden = NO;
      self.downloadRoutesButton.hidden = NO;
      self.undefinedLocationLabel.hidden = YES;
      self.selectAnotherMapButton.hidden = NO;
      [self.selectAnotherMapButton setTitle:L(@"search_select_other_map") forState:UIControlStateNormal];
      [self.selectAnotherMapButton setTitleColor:[UIColor linkBlue] forState:UIControlStateNormal];
      [self.selectAnotherMapButton setTitleColor:[UIColor white] forState:UIControlStateHighlighted];
      [self.selectAnotherMapButton setBackgroundColor:[UIColor white] forState:UIControlStateNormal];
      [self.selectAnotherMapButton setBackgroundColor:[UIColor linkBlue] forState:UIControlStateHighlighted];
      break;
    case MWMDownloadMapRequestStateRequestUnknownLocation:
      self.progressViewWrapper.hidden = YES;
      self.mapTitleLabel.hidden = YES;
      self.downloadMapButton.hidden = YES;
      self.downloadRoutesButton.hidden = YES;
      self.undefinedLocationLabel.hidden = NO;
      self.selectAnotherMapButton.hidden = NO;
      [self.selectAnotherMapButton setTitle:L(@"search_select_map") forState:UIControlStateNormal];

      [self.selectAnotherMapButton setTitleColor:[UIColor white] forState:UIControlStateNormal];
      [self.selectAnotherMapButton setBackgroundColor:[UIColor linkBlue] forState:UIControlStateNormal];
      [self.selectAnotherMapButton setBackgroundColor:[UIColor linkBlueDark] forState:UIControlStateHighlighted];
      break;
  }
}

@end
