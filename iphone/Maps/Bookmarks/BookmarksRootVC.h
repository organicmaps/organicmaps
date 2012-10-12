#import <UIKit/UIKit.h>

@class BalloonView;

@interface BookmarksRootVC : UITableViewController <UITextFieldDelegate>
{
  // @TODO store as a property to retain reference
  BalloonView * m_balloon;
}

- (id) initWithBalloonView:(BalloonView *)view;

@end
