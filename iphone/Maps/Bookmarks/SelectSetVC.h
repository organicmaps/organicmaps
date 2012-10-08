#import <UIKit/UIKit.h>

@class BalloonView;

@interface SelectSetVC : UITableViewController
{
  // @TODO store as a property to retain reference
  BalloonView * m_balloon;
}

- (id) initWithBalloonView:(BalloonView *)view;

@end
