#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMPageController.h"
#import "MWMWhatsNewEditorController.h"

#include "platform/local_country_file_utils.hpp"

@interface MWMWhatsNewEditorController ()

@property (weak, nonatomic) IBOutlet UIView * containerView;
@property (weak, nonatomic) IBOutlet UIImageView * image;
@property (weak, nonatomic) IBOutlet UILabel * alertTitle;
@property (weak, nonatomic) IBOutlet UILabel * alertText;
@property (weak, nonatomic) IBOutlet UIButton * primaryButton;
@property (weak, nonatomic) IBOutlet UIButton * secondaryButton;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerWidth;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerHeight;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageMinHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageHeight;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleTopOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleImageOffset;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * buttonsSpacing;

@end

namespace
{
NSArray<TMWMWelcomeConfigBlock> * pagesConfigBlocks = @[
  [^(MWMWhatsNewEditorController * controller) {
    controller.image.image = [UIImage imageNamed:@"img_editor_upd"];
    controller.alertTitle.text = L(@"whatsnew_update_editor_title");
    if (platform::migrate::NeedMigrate())
    {
      controller.alertText.text =
          [NSString stringWithFormat:@"%@\n\n%@", L(@"whatsnew_update_editor_message"),
                                     L(@"whatsnew_update_editor_message_update")];
      [controller.primaryButton setTitle:L(@"downloader_update_all_button")
                                forState:UIControlStateNormal];
      [controller.primaryButton addTarget:controller
                                   action:@selector(openMigration)
                         forControlEvents:UIControlEventTouchUpInside];
      [controller.secondaryButton setTitle:L(@"not_now")
                                  forState:UIControlStateNormal];
      [controller.secondaryButton addTarget:controller.pageController
                                     action:@selector(close)
                           forControlEvents:UIControlEventTouchUpInside];
    }
    else
    {
      controller.alertText.text = L(@"whatsnew_update_editor_message");
      controller.secondaryButton.hidden = YES;
      controller.buttonsSpacing.priority = UILayoutPriorityDefaultLow;
      [controller.primaryButton setTitle:L(@"done")
                                forState:UIControlStateNormal];
      [controller.primaryButton addTarget:controller.pageController
                                   action:@selector(close)
                         forControlEvents:UIControlEventTouchUpInside];
    }
  } copy]
];
}  // namespace

@implementation MWMWhatsNewEditorController

+ (NSString *)udWelcomeWasShownKey
{
  return @"WhatsNewWithEditorWasShown";
}

+ (NSArray<TMWMWelcomeConfigBlock> *)pagesConfig
{
  return pagesConfigBlocks;
}

- (void)openMigration
{
  [self.pageController close];
  [MapsAppDelegate.theApp.mapViewController openMigration];
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
