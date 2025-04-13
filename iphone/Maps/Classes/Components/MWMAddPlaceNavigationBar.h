#include "geometry/point2d.hpp"

@interface MWMAddPlaceNavigationBar : SolidTouchView

+ (void)showInSuperview:(UIView *)superview
             isBusiness:(BOOL)isBusiness
               position:(m2::PointD const *)optionalPosition
              doneBlock:(MWMVoidBlock)done
            cancelBlock:(MWMVoidBlock)cancel;

@end
