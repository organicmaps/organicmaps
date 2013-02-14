#import <UIKit/UIKit.h>
#import <MessageUI/MFMessageComposeViewController.h>
#import <MessageUI/MFMailComposeViewController.h>

@class BalloonView;

@interface PlacePageVC : UITableViewController <UITextFieldDelegate, UIActionSheetDelegate, MFMailComposeViewControllerDelegate, MFMessageComposeViewControllerDelegate>
{
  BOOL m_hideNavBar;
  // @TODO store as a property to retain reference
  BalloonView * m_balloon;
  // If YES, pin should be removed when closing this dialog
  BOOL m_removePinOnClose;
}

- (id) initWithBalloonView:(BalloonView *)view;

@end
