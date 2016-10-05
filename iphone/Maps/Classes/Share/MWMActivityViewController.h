@protocol MWMPlacePageObject;

@interface MWMActivityViewController : UIActivityViewController

+ (instancetype)shareControllerForEditorViral;

+ (instancetype)shareControllerForMyPosition:(CLLocationCoordinate2D const &)location;

+ (instancetype)shareControllerForPlacePageObject:(id<MWMPlacePageObject>)object;

- (void)presentInParentViewController:(UIViewController *)parentVC anchorView:(UIView *)anchorView;

@end
