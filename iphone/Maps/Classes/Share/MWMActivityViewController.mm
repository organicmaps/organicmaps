#import "MWMActivityViewController.h"
#import "MWMEditorViralActivityItem.h"
#import "MWMShareLocationActivityItem.h"
#import "MWMSharePedestrianRoutesToastActivityItem.h"

@interface MWMActivityViewController ()

@property (weak, nonatomic) UIViewController * ownerViewController;
@property (weak, nonatomic) UIView * anchorView;

@end

@implementation MWMActivityViewController

- (instancetype)initWithActivityItem:(id<UIActivityItemSource>)activityItem
{
  return [self initWithActivityItems:@[activityItem]];
}

- (instancetype)initWithActivityItems:(NSArray *)activityItems
{
  self = [super initWithActivityItems:activityItems applicationActivities:nil];
  if (self)
    self.excludedActivityTypes = @[UIActivityTypePrint, UIActivityTypeAssignToContact, UIActivityTypeSaveToCameraRoll,
                                 UIActivityTypeAirDrop, UIActivityTypeAddToReadingList, UIActivityTypePostToFlickr,
                                 UIActivityTypePostToVimeo];
  return self;
}

+ (instancetype)shareControllerForLocationTitle:(NSString *)title location:(CLLocationCoordinate2D)location
                                     myPosition:(BOOL)myPosition
{
  MWMShareLocationActivityItem * item = [[MWMShareLocationActivityItem alloc] initWithTitle:title location:location
                                                                                 myPosition:myPosition];
  return [[self alloc] initWithActivityItem:item];
}

+ (instancetype)shareControllerForPedestrianRoutesToast
{
  MWMSharePedestrianRoutesToastActivityItem * item = [[MWMSharePedestrianRoutesToastActivityItem alloc] init];
  MWMActivityViewController * vc = [[self alloc] initWithActivityItem:item];
  if ([vc respondsToSelector:@selector(popoverPresentationController)])
    vc.popoverPresentationController.permittedArrowDirections = UIPopoverArrowDirectionDown;
  return vc;
}

+ (instancetype)shareControllerForEditorViral
{
  MWMEditorViralActivityItem * item = [[MWMEditorViralActivityItem alloc] init];
  UIImage * image = [UIImage imageNamed:@"img_sharing_editor"];
  MWMActivityViewController * vc = [[self alloc] initWithActivityItems:@[item, image]];
  if ([vc respondsToSelector:@selector(popoverPresentationController)])
    vc.popoverPresentationController.permittedArrowDirections = UIPopoverArrowDirectionDown;
  return vc;
}

- (void)presentInParentViewController:(UIViewController *)parentVC anchorView:(UIView *)anchorView
{
  self.ownerViewController = parentVC;
  self.anchorView = anchorView;
  if ([self respondsToSelector:@selector(popoverPresentationController)])
  {
    self.popoverPresentationController.sourceView = anchorView;
    self.popoverPresentationController.sourceRect = anchorView.bounds;
  }
  [parentVC presentViewController:self animated:YES completion:nil];
}

- (BOOL)shouldAutorotate
{
  return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
  return UIInterfaceOrientationMaskAll;
}

@end
