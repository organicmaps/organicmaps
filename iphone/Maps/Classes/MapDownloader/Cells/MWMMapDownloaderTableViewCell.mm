#import "MWMCircularProgress.h"
#import "MWMMapDownloaderTableViewCell.h"

@interface MWMMapDownloaderTableViewCell () <MWMCircularProgressProtocol>

@property (nonatomic) MWMCircularProgress * progressView;
@property (weak, nonatomic) IBOutlet UIView * stateWrapper;
@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UILabel * downloadSize;

@end

@implementation MWMMapDownloaderTableViewCell

#pragma mark - Properties

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.progressView = [[MWMCircularProgress alloc] initWithParentView:self.stateWrapper];
  self.progressView.delegate = self;
  [self.progressView setImage:[UIImage imageNamed:@"ic_download"] forState:MWMCircularProgressStateNormal];
  [self.progressView setImage:[UIImage imageNamed:@"ic_download"] forState:MWMCircularProgressStateSelected];
  [self.progressView setImage:[UIImage imageNamed:@"ic_close_spinner"] forState:MWMCircularProgressStateProgress];
  [self.progressView setImage:[UIImage imageNamed:@"ic_download_error"] forState:MWMCircularProgressStateFailed];
  [self.progressView setImage:[UIImage imageNamed:@"ic_check"] forState:MWMCircularProgressStateCompleted];
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.title.preferredMaxLayoutWidth = self.title.width;
  self.downloadSize.preferredMaxLayoutWidth = self.downloadSize.width;
  [super layoutSubviews];
}

- (void)setTitleText:(NSString *)text
{
  self.title.text = text;
}

- (void)setDownloadSizeText:(NSString *)text
{
  self.downloadSize.text = text;
}

#pragma mark - MWMCircularProgressDelegate

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{

}

#pragma mark - Properties

- (CGFloat)estimatedHeight
{
  return 52.0;
}

@end
