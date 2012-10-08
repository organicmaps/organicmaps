#import <UIKit/UIKit.h>

@class BalloonView;

@interface AddSetVC : UITableViewController <UITextFieldDelegate>
{
  // @TODO store as a property to retain reference
  BalloonView * m_balloon;
  // Used to correctly display the most root view after pressing Save
  // We don't use self.navigationController because it's another one :)
  UINavigationController * m_rootNavigationController;
}

- (id) initWithBalloonView:(BalloonView *)view andRootNavigationController:(UINavigationController *)navC;

@end
