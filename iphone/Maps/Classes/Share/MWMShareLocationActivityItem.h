@interface MWMShareLocationActivityItem : NSObject <UIActivityItemSource>

- (instancetype)initWithTitle:(NSString *)title location:(CLLocationCoordinate2D)location myPosition:(BOOL)myPosition;

@end
