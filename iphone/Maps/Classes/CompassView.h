#import <UIKit/UIKit.h>

// Custom view which drows an arrow
@interface CompassView : UIView
// Rotation angle in radians (decart system)
@property (nonatomic) float angle;
// NO by default - do not display anything
// If set to YES - use angle to display the arrow
@property (nonatomic) BOOL showArrow;
@property (nonatomic) UIColor * color;

@end
