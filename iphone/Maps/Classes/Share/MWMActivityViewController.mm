#import "MWMActivityViewController.h"
#import "MWMEditorViralActivityItem.h"
#import "MWMShareActivityItem.h"

@interface MWMActivityViewController ()

@property(weak, nonatomic) UIViewController *ownerViewController;
@property(weak, nonatomic) UIView *anchorView;

@end

@implementation MWMActivityViewController

- (instancetype)initWithActivityItem:(id<UIActivityItemSource>)activityItem {
  return [self initWithActivityItems:@[activityItem]];
}

- (instancetype)initWithActivityItems:(NSArray *)activityItems {
  self = [super initWithActivityItems:activityItems applicationActivities:nil];
  if (self)
    self.excludedActivityTypes = @[
      UIActivityTypePrint, UIActivityTypeAssignToContact, UIActivityTypeSaveToCameraRoll,
      UIActivityTypeAddToReadingList, UIActivityTypePostToFlickr, UIActivityTypePostToVimeo
    ];
  return self;
}

+ (instancetype)shareControllerForMyPosition:(CLLocationCoordinate2D)location {
  MWMShareActivityItem *item = [[MWMShareActivityItem alloc] initForMyPositionAtLocation:location];
  MWMActivityViewController *shareVC = [[self alloc] initWithActivityItem:item];
  shareVC.excludedActivityTypes = [shareVC.excludedActivityTypes arrayByAddingObject:UIActivityTypeAirDrop];
  return shareVC;
}

+ (instancetype)shareControllerForPlacePage:(PlacePageData *)data {
  MWMShareActivityItem *item = [[MWMShareActivityItem alloc] initForPlacePage:data];
  MWMActivityViewController *shareVC = [[self alloc] initWithActivityItem:item];
  shareVC.excludedActivityTypes = [shareVC.excludedActivityTypes arrayByAddingObject:UIActivityTypeAirDrop];
  return shareVC;
}

+ (instancetype)shareControllerForURL:(NSURL *)url
                              message:(NSString *)message
                    completionHandler:(UIActivityViewControllerCompletionWithItemsHandler)completionHandler {
  NSMutableArray *items = [NSMutableArray arrayWithObject:message];
  if (url) {
    [items addObject:url];
  }

  MWMActivityViewController *shareVC = [[self alloc] initWithActivityItems:items.copy];
  shareVC.excludedActivityTypes = [shareVC.excludedActivityTypes arrayByAddingObject:UIActivityTypePostToFacebook];
  shareVC.completionWithItemsHandler = completionHandler;
  return shareVC;
}

+ (instancetype)shareControllerForEditorViral {
  MWMEditorViralActivityItem *item = [[MWMEditorViralActivityItem alloc] init];
  UIImage *image = [UIImage imageNamed:@"img_sharing_editor"];
  MWMActivityViewController *vc = [[self alloc] initWithActivityItems:@[item, image]];
  vc.popoverPresentationController.permittedArrowDirections = UIPopoverArrowDirectionDown;
  return vc;
}

- (void)presentInParentViewController:(UIViewController *)parentVC anchorView:(UIView *)anchorView {
  self.ownerViewController = parentVC;
  self.anchorView = anchorView;
  self.popoverPresentationController.sourceView = anchorView;
  self.popoverPresentationController.sourceRect = anchorView.bounds;
  [parentVC presentViewController:self animated:YES completion:nil];
}

@end
