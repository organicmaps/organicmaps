#import "MWMMapDownloaderPlaceTableViewCell.h"

@interface MWMMapDownloaderTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UILabel * downloadSize;

@end

@interface MWMMapDownloaderPlaceTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * area;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleBottomOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleLeadingOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleSizeOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * downloadSizeTrailingOffset;

@end

@implementation MWMMapDownloaderPlaceTableViewCell

- (void)layoutSubviews
{
  CGFloat const preferredMaxLayoutWidth =
      CGRectGetWidth(self.bounds) - self.titleLeadingOffset.constant -
      self.titleSizeOffset.constant - CGRectGetWidth(self.downloadSize.bounds) -
      self.downloadSizeTrailingOffset.constant;
  self.title.preferredMaxLayoutWidth = preferredMaxLayoutWidth;
  self.area.preferredMaxLayoutWidth = preferredMaxLayoutWidth;
  [super layoutSubviews];
}

- (void)setAreaText:(NSString *)text
{
  self.area.text = text;
  BOOL const isAreaHidden = (text.length == 0);
  self.area.hidden = isAreaHidden;
  self.titleBottomOffset.priority = isAreaHidden ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
}

#pragma mark - Properties

- (CGFloat)estimatedHeight
{
  return 62.0;
}

@end
