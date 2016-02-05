#import "MWMCircularProgress.h"
#import "MWMMapDownloaderTableViewCell.h"

@interface MWMMapDownloaderTableViewCell () <MWMCircularProgressProtocol>

@property (nonatomic) MWMCircularProgress * progressView;
@property (weak, nonatomic) IBOutlet UIView * stateWrapper;
@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UILabel * downloadSize;
@property (weak, nonatomic) IBOutlet UIView * separator;

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

- (void)setTitleText:(NSString *)text
{
  self.title.text = text;
}

- (void)setDownloadSizeText:(NSString *)text
{
  self.downloadSize.text = text;
}

- (void)setLastCell:(BOOL)isLast
{
  self.separator.hidden = isLast;
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated
{
  UIColor * color = self.separator.backgroundColor;
  [super setSelected:selected animated:animated];
  self.separator.backgroundColor = color;
}

- (void)setHighlighted:(BOOL)highlighted animated:(BOOL)animated{
  UIColor * color = self.separator.backgroundColor;
  [super setHighlighted:highlighted animated:animated];
  self.separator.backgroundColor = color;
}

#pragma mark - MWMCircularProgressDelegate

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{

}

@end
