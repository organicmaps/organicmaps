#import <UIKit/UIKit.h>

@interface BalloonView : NSObject
{
  UIImageView * m_pinView;
  UIImageView * m_buttonsView;
  id m_target;
  SEL m_selector;
}

@property(nonatomic,assign,readonly) BOOL isDisplayed;

- (id) initWithTarget:(id)target andSelector:(SEL)selector;
- (void) showInView:(UIView *)view atPoint:(CGPoint)pt;
- (void) hide;

@end
