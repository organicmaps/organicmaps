#import <UIKit/UIKit.h>

#include "../../geometry/point2d.hpp"


@interface BalloonView : NSObject
{
  UIImageView * m_pinView;
  UIImageView * m_buttonsView;
  id m_target;
  SEL m_selector;
}

@property(nonatomic, assign, readonly) BOOL isDisplayed;
@property(nonatomic, assign) m2::PointD glbPos;

- (id) initWithTarget:(id)target andSelector:(SEL)selector;
- (void) showInView:(UIView *)view atPoint:(CGPoint)pt;
- (void) updatePosition:(UIView *)view atPoint:(CGPoint)pt;
- (void) hide;

@end
