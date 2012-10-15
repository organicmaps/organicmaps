#import <UIKit/UIKit.h>

@class BalloonView;

@interface PlacePageVC : UITableViewController <UITextFieldDelegate>
{
  BOOL m_hideNavBar;
  // @TODO store as a property to retain reference
  BalloonView * m_balloon;
  // If YES, pin should be removed when closing this dialog
  BOOL m_removePinOnClose;
}

- (id) initWithBalloonView:(BalloonView *)view;

@end
