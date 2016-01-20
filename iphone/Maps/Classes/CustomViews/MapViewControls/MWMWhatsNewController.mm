#import "MWMPageController.h"
#import "MWMWhatsNewController.h"

@interface MWMWhatsNewController ()

@property (weak, nonatomic) IBOutlet UIView * containerView;
@property (weak, nonatomic) IBOutlet UIImageView * image;
@property (weak, nonatomic) IBOutlet UILabel * alertTitle;
@property (weak, nonatomic) IBOutlet UILabel * alertText;
@property (weak, nonatomic) IBOutlet UIButton * nextPageButton;
@property (weak, nonatomic, readwrite) IBOutlet UIButton * enableButton;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerWidth;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerHeight;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageMinHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageHeight;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleTopOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleImageOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * betweenButtonsOffset;

@end

@implementation MWMWhatsNewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configureFirstPage];
  [self updateForSize:self.parentViewController.view.size];
}

- (void)configureFirstPage
{
  self.image.image = [UIImage imageNamed:@"img_nightmode"];
  self.alertTitle.text = L(@"whats_new_night_caption");
  self.alertText.text = L(@"whats_new_night_body");
  [self.enableButton setTitle:L(@"whats_new_3d_buildings_on") forState:UIControlStateNormal];
  [self.nextPageButton setTitle:L(@"done") forState:UIControlStateNormal];
  [self.nextPageButton addTarget:self.pageController action:@selector(close) forControlEvents:UIControlEventTouchUpInside];
  [self.enableButton addTarget:self.pageController action:@selector(enableFirst:) forControlEvents:UIControlEventTouchUpInside];
}

- (void)configureSecondPage
{
  self.image.image = [UIImage imageNamed:@"img_perspective_view"];
  self.alertTitle.text = L(@"whats_new_3d_title");
  self.alertText.text = L(@"whats_new_3d_subtitle");
  [self.enableButton setTitle:L(@"whats_new_3d_on") forState:UIControlStateNormal];
  [self.nextPageButton setTitle:L(@"done") forState:UIControlStateNormal];
  [self.nextPageButton addTarget:self.pageController action:@selector(close) forControlEvents:UIControlEventTouchUpInside];
  [self.enableButton addTarget:self.pageController action:@selector(enableSecond) forControlEvents:UIControlEventTouchUpInside];
  self.enableButton.selected = NO;
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
  CGSize const iPadSize = {520.0, 534.0};
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
  BOOL const isPortrait = (!IPAD && size.height > size.width);
  self.betweenButtonsOffset.constant = isPortrait ? 16 : 8;
}

@end
