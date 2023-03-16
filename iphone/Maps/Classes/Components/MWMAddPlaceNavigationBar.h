#include "geometry/point2d.hpp"

@interface MWMAddPlaceNavigationBar : SolidTouchView

+ (void)showInSuperview:(UIView *)superview
             isBusiness:(BOOL)isBusiness
          applyPosition:(BOOL)applyPosition
               position:(m2::PointD const &)position
              doneBlock:(MWMVoidBlock)done
            cancelBlock:(MWMVoidBlock)cancel;

@end
