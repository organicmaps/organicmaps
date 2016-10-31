#import "MWMWhatsNewUberController.h"
#import "MWMPageController.h"

@interface MWMWhatsNewUberController ()

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
                                                          [^(MWMWhatsNewUberController * controller) {
                                                            controller.image.image = [UIImage imageNamed:@"img_whatsnew_uber"];
                                                            controller.alertTitle.text = L(@"whatsnew_uber_header");
                                                            controller.alertText.text = L(@"whatsnew_uber_message");
                                                            [controller.nextPageButton setTitle:L(@"done") forState:UIControlStateNormal];
                                                            [controller.nextPageButton addTarget:controller.pageController
                                                                                          action:@selector(close)
                                                                                forControlEvents:UIControlEventTouchUpInside];
                                                          } copy]];
  
}  // namespace

@implementation MWMWhatsNewUberController

+ (NSString *)udWelcomeWasShownKey { return @"MWMWhatsNewUberController"; }
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
