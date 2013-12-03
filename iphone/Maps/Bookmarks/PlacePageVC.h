#import <UIKit/UIKit.h>
#import "MessageComposeViewController.h"
#import "MailComposeViewController.h"
#import "ColorPickerView.h"
#import "../../../map/bookmark.hpp"


namespace search { struct AddressInfo; }
namespace url_scheme { struct ApiPoint; }

@interface PlacePageVC : UITableViewController <UITextFieldDelegate, UIActionSheetDelegate, MFMailComposeViewControllerDelegate, MFMessageComposeViewControllerDelegate, UITextViewDelegate, UIGestureRecognizerDelegate, ColorPickerDelegate>

- (id) initWithInfo:(search::AddressInfo const &)info point:(CGPoint)point;
- (id) initWithApiPoint:(url_scheme::ApiPoint const &)apiPoint;
- (id) initWithBookmark:(BookmarkAndCategory)bmAndCat;
- (id) initWithName:(NSString *)name andGlobalPoint:(CGPoint)point;
@end
