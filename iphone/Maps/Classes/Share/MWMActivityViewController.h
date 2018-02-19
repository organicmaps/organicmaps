@protocol MWMPlacePageObject;

@interface MWMActivityViewController : UIActivityViewController

+ (instancetype)shareControllerForEditorViral;

+ (instancetype)shareControllerForMyPosition:(CLLocationCoordinate2D)location;

+ (instancetype)shareControllerForPlacePageObject:(id<MWMPlacePageObject>)object;

+ (instancetype)shareControllerForURL:(NSURL *)url
                              message:(NSString *)message
                    completionHandler:
                        (UIActivityViewControllerCompletionWithItemsHandler)completionHandler;

- (void)presentInParentViewController:(UIViewController *)parentVC anchorView:(UIView *)anchorView;

@end
