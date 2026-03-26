#import "MWMActivityViewController.h"
#import <LinkPresentation/LPLinkMetadata.h>
#import "MWMEditorViralActivityItem.h"
#import "MWMShareActivityItem.h"

#pragma mark - File Share Activity Item

@interface MWMFileShareActivityItem : NSObject <UIActivityItemSource>

@property(nonatomic) NSURL * fileURL;
@property(nonatomic, copy) NSString * displayName;

@end

@implementation MWMFileShareActivityItem

- (instancetype)initWithFileURL:(NSURL *)url displayName:(NSString *)displayName
{
  self = [super init];
  if (self)
  {
    _fileURL = url;
    _displayName = [displayName copy];
  }
  return self;
}

- (id)activityViewControllerPlaceholderItem:(UIActivityViewController *)activityViewController
{
  return self.fileURL;
}

- (id)activityViewController:(UIActivityViewController *)activityViewController
         itemForActivityType:(NSString *)activityType
{
  NSItemProvider * provider = [[NSItemProvider alloc] initWithContentsOfURL:self.fileURL];
  if (provider)
  {
    NSString * ext = self.fileURL.pathExtension;
    NSString * name = self.displayName;
    // Avoid double extension (e.g. "Track.gpx.gpx").
    if (ext.length > 0 && ![name.pathExtension isEqualToString:ext])
      name = [NSString stringWithFormat:@"%@.%@", name, ext];
    provider.suggestedName = name;
    return provider;
  }
  return self.fileURL;
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController
              subjectForActivityType:(NSString *)activityType
{
  return self.displayName;
}

- (LPLinkMetadata *)activityViewControllerLinkMetadata:(UIActivityViewController *)activityViewController
    API_AVAILABLE(ios(13.0))
{
  LPLinkMetadata * metadata = [[LPLinkMetadata alloc] init];
  metadata.originalURL = self.fileURL;
  metadata.title = self.displayName;
  metadata.iconProvider = [[NSItemProvider alloc] initWithObject:[UIImage imageNamed:@"imgLogo"]];
  return metadata;
}

@end

#pragma mark - Activity View Controller

@interface MWMActivityViewController ()

@property(weak, nonatomic) UIViewController * ownerViewController;
@property(weak, nonatomic) UIView * anchorView;

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
    self.excludedActivityTypes = @[
      UIActivityTypePrint, UIActivityTypeAssignToContact, UIActivityTypeSaveToCameraRoll,
      UIActivityTypeAddToReadingList, UIActivityTypePostToFlickr, UIActivityTypePostToVimeo
    ];
  return self;
}

+ (instancetype)shareControllerForMyPosition:(CLLocationCoordinate2D)location
{
  MWMShareActivityItem * item = [[MWMShareActivityItem alloc] initForMyPositionAtLocation:location];
  MWMActivityViewController * shareVC = [[self alloc] initWithActivityItem:item];
  shareVC.excludedActivityTypes = [shareVC.excludedActivityTypes arrayByAddingObject:UIActivityTypeAirDrop];
  return shareVC;
}

+ (instancetype)shareControllerForPlacePage:(PlacePageData *)data
{
  MWMShareActivityItem * item = [[MWMShareActivityItem alloc] initForPlacePage:data];
  MWMActivityViewController * shareVC = [[self alloc] initWithActivityItem:item];
  shareVC.excludedActivityTypes = [shareVC.excludedActivityTypes arrayByAddingObject:UIActivityTypeAirDrop];
  return shareVC;
}

+ (instancetype)shareControllerForURL:(NSURL *)url
                              message:(NSString *)message
                          displayName:(nullable NSString *)displayName
                    completionHandler:(UIActivityViewControllerCompletionWithItemsHandler)completionHandler
{
  MWMActivityViewController * shareVC;
  if (url.isFileURL)
  {
    if (!displayName)
      displayName = url.lastPathComponent.stringByDeletingPathExtension;
    MWMFileShareActivityItem * item = [[MWMFileShareActivityItem alloc] initWithFileURL:url displayName:displayName];
    shareVC = [[self alloc] initWithActivityItems:@[item, message]];
  }
  else
  {
    NSMutableArray * items = [NSMutableArray arrayWithObject:message];
    if (url)
      [items addObject:url];
    shareVC = [[self alloc] initWithActivityItems:items.copy];
  }
  shareVC.excludedActivityTypes = [shareVC.excludedActivityTypes arrayByAddingObject:UIActivityTypePostToFacebook];
  shareVC.completionWithItemsHandler = completionHandler;
  return shareVC;
}

+ (instancetype)shareControllerForEditorViral
{
  MWMEditorViralActivityItem * item = [[MWMEditorViralActivityItem alloc] init];
  UIImage * image = [UIImage imageNamed:@"img_sharing_editor"];
  MWMActivityViewController * vc = [[self alloc] initWithActivityItems:@[item, image]];
  vc.popoverPresentationController.permittedArrowDirections = UIPopoverArrowDirectionDown;
  return vc;
}

- (void)presentInParentViewController:(UIViewController *)parentVC anchorView:(UIView *)anchorView
{
  self.ownerViewController = parentVC;
  self.anchorView = anchorView;
  self.popoverPresentationController.sourceView = anchorView;
  self.popoverPresentationController.sourceRect = anchorView.bounds;
  [parentVC presentViewController:self animated:YES completion:nil];
}

@end
