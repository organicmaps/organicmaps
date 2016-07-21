@class MWMPlacePageEntity;

@interface MWMActivityViewController : UIActivityViewController

+ (instancetype)shareControllerForEditorViral;

+ (instancetype)shareControllerForMyPosition:(CLLocationCoordinate2D const &)location;

+ (instancetype)shareControllerForPlacePageObject:(MWMPlacePageEntity *)entity;

- (void)presentInParentViewController:(UIViewController *)parentVC anchorView:(UIView *)anchorView;

@end
