@protocol MWMPlacePageObject;
@class PlacePageData;

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(ActivityViewController)
@interface MWMActivityViewController : UIActivityViewController

+ (instancetype)shareControllerForEditorViral;

+ (instancetype)shareControllerForMyPosition:(CLLocationCoordinate2D)location;

+ (instancetype)shareControllerForPlacePage:(PlacePageData *)data;

+ (instancetype)shareControllerForURL:(nullable NSURL *)url
                              message:(NSString *)message
                    completionHandler:(nullable UIActivityViewControllerCompletionWithItemsHandler)completionHandler;

- (void)presentInParentViewController:(UIViewController *)parentVC anchorView:(nullable UIView *)anchorView;

@end

NS_ASSUME_NONNULL_END
