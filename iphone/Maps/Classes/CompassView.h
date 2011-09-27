#import <UIKit/UIKit.h>

// Custom view which drows an arrow
@interface CompassView : UIView
{
  float m_angle;
}
// Rotation angle in radians (decart system)
@property (nonatomic, assign) float angle;

@end
