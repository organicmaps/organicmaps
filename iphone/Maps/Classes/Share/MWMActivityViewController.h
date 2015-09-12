@interface MWMActivityViewController : UIActivityViewController

+ (instancetype)shareControllerForLocationTitle:(NSString *)title location:(CLLocationCoordinate2D)location
                                     myPosition:(BOOL)myPosition;
+ (instancetype)shareControllerForPedestrianRoutesToast;

- (void)presentInParentViewController:(UIViewController *)parentVC anchorView:(UIView *)anchorView;

@end
