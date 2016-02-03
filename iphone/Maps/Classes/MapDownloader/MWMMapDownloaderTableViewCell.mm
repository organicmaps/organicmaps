#import "MWMCircularProgress.h"
#import "MWMMapDownloaderTableViewCell.h"

@interface MWMMapDownloaderTableViewCell () <MWMCircularProgressProtocol>

@property (nonatomic) MWMCircularProgress * progressView;

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

- (CGFloat)estimatedHeight
{
  return 52.0;
}

#pragma mark - MWMCircularProgressDelegate

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{

}

@end
