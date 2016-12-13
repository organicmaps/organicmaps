#import "MWMWhatsNewTrafficController.h"
#import "MWMPageController.h"

@interface MWMWhatsNewTrafficController ()

@property(weak, nonatomic) IBOutlet UIView * containerView;
@property(weak, nonatomic) IBOutlet UIImageView * image;
@property(weak, nonatomic) IBOutlet UILabel * alertTitle;
@property(weak, nonatomic) IBOutlet UILabel * alertText;
@property(weak, nonatomic) IBOutlet UIButton * nextPageButton;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * containerWidth;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * containerHeight;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * imageMinHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * imageHeight;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * titleTopOffset;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * titleImageOffset;

@end

namespace
{
NSArray<TMWMWelcomeConfigBlock> * pagesConfigBlocks = @[
  [^(MWMWhatsNewTrafficController * controller) {
    controller.image.image = [UIImage imageNamed:@"whats_new_traffic"];
    controller.alertTitle.text = L(@"whatsnew_traffic");
    controller.alertText.text = L(@"whatsnew_traffic_text");
    [controller.nextPageButton setTitle:L(@"whats_new_next_button") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller.pageController
                                  action:@selector(nextPage)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy],
  [^(MWMWhatsNewTrafficController * controller) {
    controller.image.image = [UIImage imageNamed:@"whats_new_traffic_roaming"];
    controller.alertTitle.text = L(@"whatsnew_traffic_roaming");
    controller.alertText.text = L(@"whatsnew_traffic_roaming_text");
    [controller.nextPageButton setTitle:L(@"whats_new_next_button") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller.pageController
                                  action:@selector(nextPage)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy],
  [^(MWMWhatsNewTrafficController * controller) {
    controller.image.image = [UIImage imageNamed:@"whats_new_update_uber"];
    controller.alertTitle.text = L(@"whatsnew_order_taxi");
    controller.alertText.text = L(@"whatsnew_order_taxi_text");
    [controller.nextPageButton setTitle:L(@"done") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller.pageController
                                  action:@selector(close)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy]
];

}  // namespace

@implementation MWMWhatsNewTrafficController

+ (NSString *)udWelcomeWasShownKey { return @"MWMWhatsNewTrafficController"; }
+ (NSArray<TMWMWelcomeConfigBlock> *)pagesConfig { return pagesConfigBlocks; }
- (IBAction)close { [self.pageController close]; }
#pragma mark - Properties

- (void)setSize:(CGSize)size
{
  super.size = size;
  CGSize const newSize = super.size;
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
