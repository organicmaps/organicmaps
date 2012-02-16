#import <UIKit/UIKit.h>

// Custom view which drows an arrow
@interface CompassView : UIView
// Rotation angle in radians (decart system)
@property (nonatomic, assign) float angle;
// Image to display instead of compass arrow
@property (nonatomic, retain) UIImage * image;
@end
