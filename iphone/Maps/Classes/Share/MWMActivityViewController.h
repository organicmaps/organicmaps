@protocol MWMPlacePageObject;
@class PlacePageData;

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(ActivityViewController)
@interface MWMActivityViewController : UIActivityViewController

+ (nullable instancetype)shareControllerForEditorViral;

+ (nullable instancetype)shareControllerForMyPosition:(CLLocationCoordinate2D)location;

+ (nullable instancetype)shareControllerForPlacePage:(PlacePageData *)data;

+ (nullable instancetype)shareControllerForURL:(nullable NSURL *)url
                              message:(NSString *)message
                    completionHandler:(nullable UIActivityViewControllerCompletionWithItemsHandler)completionHandler;

- (void)presentInParentViewController:(UIViewController *)parentVC anchorView:(nullable UIView *)anchorView;

@end

NS_ASSUME_NONNULL_END
