#import "MWMPageController.h"
#import "MWMWhatsNewController.h"

@interface MWMWhatsNewController ()

@property (weak, nonatomic) IBOutlet UIView * containerView;
@property (weak, nonatomic) IBOutlet UIImageView * image;
@property (weak, nonatomic) IBOutlet UILabel * alertTitle;
@property (weak, nonatomic) IBOutlet UILabel * alertText;
@property (weak, nonatomic) IBOutlet UIButton * button;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerWidth;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerHeight;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageMinHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageHeight;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleTopOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleImageOffset;

@end

@implementation MWMWhatsNewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  if (self.pageIndex == 0)
    [self configureFirstPage];
  else
    [self configureSecondPage];
  [self updateForSize:self.parentViewController.view.size];
}

- (void)configureFirstPage
{
  self.image.image = [UIImage imageNamed:@"img_tts"];
  self.alertTitle.text = L(@"whats_new_where_to_turn");
  self.alertText.text = L(@"whats_new_voice_instructions");
  [self.button setTitle:L(@"whats_new_next") forState:UIControlStateNormal];
  [self.button addTarget:self.pageController action:@selector(nextPage) forControlEvents:UIControlEventTouchUpInside];
}

- (void)configureSecondPage
{
  self.image.image = [UIImage imageNamed:@"img_p2p"];
  self.alertTitle.text = L(@"whats_new_between_any_points");
  self.alertText.text = L(@"whats_new_happy_day");
  [self.button setTitle:L(@"done") forState:UIControlStateNormal];
  [self.button addTarget:self.pageController action:@selector(close) forControlEvents:UIControlEventTouchUpInside];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext>  _Nonnull context)
  {
    [self updateForSize:size];
  }
  completion:^(id<UIViewControllerTransitionCoordinatorContext>  _Nonnull context) { }];
}

- (void)updateForSize:(CGSize)size
{
  CGSize const iPadSize = {520.0, 600.0};
  CGSize const newSize = IPAD ? iPadSize : size;
  self.parentViewController.view.size = newSize;
  CGFloat const width = newSize.width;
  CGFloat const height = newSize.height;
  BOOL const hideImage = (self.imageHeight.multiplier * height <= self.imageMinHeight.constant);
  self.titleImageOffset.priority =
  hideImage ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
  self.image.hidden = hideImage;
  self.containerWidth.constant = width;
  self.containerHeight.constant = height;
}

@end
