#import "MWMWhatsNewProfileBookingController.h"
#import "MWMPageController.h"

@interface MWMWhatsNewProfileBookingController ()

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
  [^(MWMWhatsNewProfileBookingController * controller) {
    controller.image.image = [UIImage imageNamed:@"img_whatsnew_route_profile"];
    controller.alertTitle.text = L(@"whats_new_route_profile_title");
    controller.alertText.text = L(@"whats_new_route_profile_message");
    [controller.nextPageButton setTitle:L(@"whats_new_next_button") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller.pageController
                                  action:@selector(nextPage)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy],
  [^(MWMWhatsNewProfileBookingController * controller) {
    controller.image.image = [UIImage imageNamed:@"img_whatsnew_booking_improved"];
    controller.alertTitle.text = L(@"whats_new_booking_improve_title");
    controller.alertText.text = L(@"whats_new_booking_improve_message");
    [controller.nextPageButton setTitle:L(@"done") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller.pageController
                                  action:@selector(close)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy]
];

}  // namespace

@implementation MWMWhatsNewProfileBookingController

+ (NSString *)udWelcomeWasShownKey { return @"WhatsNewProfileBookingWasShown"; }
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
