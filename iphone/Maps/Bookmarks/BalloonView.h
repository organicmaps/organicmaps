#import <UIKit/UIKit.h>

#include "../../geometry/point2d.hpp"


@interface BalloonView : NSObject
{
  UIImageView * m_pinView;
  UITableViewCell * m_titleView;
  id m_target;
  SEL m_selector;
}

@property(nonatomic, retain) NSString * title;
@property(nonatomic, retain) NSString * description;
@property(nonatomic, assign, readonly) BOOL isDisplayed;
@property(nonatomic, assign) CGPoint glbPos;

- (id) initWithTarget:(id)target andSelector:(SEL)selector;
- (void) showInView:(UIView *)view atPoint:(CGPoint)pt;
- (void) updatePosition:(UIView *)view atPoint:(CGPoint)pt;
- (void) hide;

@end
