#import <UIKit/UIKit.h>

@class BalloonView;

@interface SelectSetVC : UITableViewController
{
  // @TODO store as a property to retain reference
  BalloonView * m_balloon;
  
  BOOL m_editModeEnabled;
}

- (id) initWithBalloonView:(BalloonView *)view andEditMode:(BOOL)enabled;

@end
