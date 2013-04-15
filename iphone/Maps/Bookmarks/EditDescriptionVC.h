#import <UIKit/UIKit.h>

@class BalloonView;

@interface EditDescriptionVC : UIViewController
{
  // @TODO store as a property to retain reference
  // Used to pass description (notes) for editing
  BalloonView * m_balloon;
}

- (id) initWithBalloonView:(BalloonView *)view;

@end
