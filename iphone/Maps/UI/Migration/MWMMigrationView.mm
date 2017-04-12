#import "MWMCommon.h"
#import "MWMMigrationView.h"

@interface MWMMigrationView ()

@property (weak, nonatomic) IBOutlet UIImageView * image;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageMinHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageHeight;

@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleTopOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleImageOffset;

@property (weak, nonatomic) IBOutlet UILabel * text;
@property (weak, nonatomic) IBOutlet UILabel * info;

@property (weak, nonatomic) IBOutlet UIButton * primaryButton;
@property (weak, nonatomic) IBOutlet UIButton * secondaryButton;
@property (weak, nonatomic) IBOutlet UIView * spinnerView;

@property (nonatomic) MWMCircularProgress * spinner;

@end

@implementation MWMMigrationView

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.state = MWMMigrationViewState::Default;
  self.secondaryButton.titleLabel.textAlignment = NSTextAlignmentCenter;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.title.preferredMaxLayoutWidth = self.title.width;
  self.text.preferredMaxLayoutWidth = self.text.width;
  self.info.preferredMaxLayoutWidth = self.info.width;
  [super layoutSubviews];
}

- (void)setFrame:(CGRect)frame
{
  [self updateForSize:frame.size];
  super.frame = frame;
}

- (void)updateForSize:(CGSize)size
{
  BOOL const hideImage = (self.imageHeight.multiplier * size.height <= self.imageMinHeight.constant);
  self.titleImageOffset.priority = hideImage ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
  self.image.hidden = hideImage;
  [self layoutIfNeeded];
}

- (void)configDefaultState
{
  [self stopSpinner];
  self.info.hidden = YES;
  self.info.textColor = [UIColor blackHintText];
  self.primaryButton.enabled = YES;
  self.secondaryButton.enabled = YES;
  self.primaryButton.hidden = NO;
  self.secondaryButton.hidden = NO;
}

- (void)startSpinner
{
  self.spinnerView.hidden = NO;
  self.spinner = [MWMCircularProgress downloaderProgressForParentView:self.spinnerView];
  self.spinner.delegate = self.delegate;
  [self.spinner setInvertColor:YES];
  self.spinner.state = MWMCircularProgressStateSpinner;
}

- (void)stopSpinner
{
  self.spinnerView.hidden = YES;
  self.spinner = nil;
}

- (void)setProgress:(CGFloat)progress
{
  self.spinner.progress = progress;
}

#pragma mark - Properties

- (void)setState:(MWMMigrationViewState)state
{
  [self layoutIfNeeded];
  [self configDefaultState];
  switch (state)
  {
    case MWMMigrationViewState::Default:
      self.primaryButton.hidden = NO;
      self.secondaryButton.hidden = NO;
      break;
    case MWMMigrationViewState::Processing:
      self.info.hidden = NO;
      self.info.text = [NSString stringWithFormat:@"%@ %@", L(@"downloader_downloading"), self.nodeLocalName];
      [self startSpinner];
      self.primaryButton.enabled = NO;
      self.primaryButton.hidden = YES;
      self.secondaryButton.hidden = YES;
      break;
    case MWMMigrationViewState::ErrorNoConnection:
      self.info.hidden = NO;
      self.info.textColor = [UIColor red];
      self.info.text = L(@"common_check_internet_connection_dialog");
      self.primaryButton.hidden = YES;
      self.secondaryButton.hidden = YES;
      break;
    case MWMMigrationViewState::ErrorNoSpace:
      self.info.hidden = NO;
      self.info.textColor = [UIColor red];
      self.info.text = L(@"migration_no_space_message");
      self.primaryButton.hidden = YES;
      self.secondaryButton.hidden = YES;
      break;
  }
  _state = state;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ [self layoutIfNeeded]; }];
}

@end
