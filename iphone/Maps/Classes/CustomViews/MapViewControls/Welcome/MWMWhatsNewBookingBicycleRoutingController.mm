#import "MWMPageController.h"
#import "MWMWhatsNewBookingBicycleRoutingController.h"

@interface MWMWhatsNewBookingBicycleRoutingController ()

@property (weak, nonatomic) IBOutlet UIView * containerView;
@property (weak, nonatomic) IBOutlet UIImageView * image;
@property (weak, nonatomic) IBOutlet UILabel * alertTitle;
@property (weak, nonatomic) IBOutlet UILabel * alertText;
@property (weak, nonatomic) IBOutlet UIButton * nextPageButton;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerWidth;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerHeight;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageMinHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageHeight;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleTopOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleImageOffset;

- (void)tryBooking;

@end

namespace
{
NSArray<TMWMWelcomeConfigBlock> * pagesConfigBlocks = @[
  [^(MWMWhatsNewBookingBicycleRoutingController * controller) {
    controller.image.image = [UIImage imageNamed:@"img_booking"];
    controller.alertTitle.text = L(@"whatsnew_booking_header");
    controller.alertText.text = L(@"whatsnew_booking_message");
    controller.nextPageButton.hidden = NO;
    [controller.nextPageButton setTitle:L(@"button_try") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller
                                  action:@selector(tryBooking)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy],
  [^(MWMWhatsNewBookingBicycleRoutingController * controller) {
    controller.image.image = [UIImage imageNamed:@"img_bikecycle_navigation"];
    controller.alertTitle.text = L(@"whatsnew_cycle_navigation_header");
    controller.alertText.text = L(@"whatsnew_cycle_navigation_message");
    controller.nextPageButton.hidden = YES;
  } copy]
];
}  // namespace

@implementation MWMWhatsNewBookingBicycleRoutingController

+ (NSString *)udWelcomeWasShownKey
{
  return @"WhatsNewBookingBicycleRoutingWasShown";
}

+ (NSArray<TMWMWelcomeConfigBlock> *)pagesConfig
{
  return pagesConfigBlocks;
}

- (void)tryBooking
{
  [self.pageController mapSearchText:[L(@"hotel") stringByAppendingString:@" "] forInputLocale:nil];
  [self close];
}

- (IBAction)close
{
  [self.pageController close];
}

#pragma mark - Properties

- (void)setSize:(CGSize)size
{
  super.size = size;
  CGSize const newSize = super.size;
  CGFloat const width = newSize.width;
  CGFloat const height = newSize.height;
  BOOL const hideImage = (self.imageHeight.multiplier * height <= self.imageMinHeight.constant);
  self.titleImageOffset.priority = hideImage ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
  self.image.hidden = hideImage;
  self.containerWidth.constant = width;
  self.containerHeight.constant = height;
}

@end
