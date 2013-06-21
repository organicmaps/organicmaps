#import <UIKit/UIKit.h>
#import <MessageUI/MFMessageComposeViewController.h>
#import <MessageUI/MFMailComposeViewController.h>
#import "../../../map/bookmark.hpp"


namespace search { struct AddressInfo; }
namespace url_scheme { struct ApiPoint; }

@interface PlacePageVC : UITableViewController <UITextFieldDelegate, UIActionSheetDelegate, MFMailComposeViewControllerDelegate, MFMessageComposeViewControllerDelegate, UIPickerViewDelegate, UITextViewDelegate>

- (id) initWithInfo:(search::AddressInfo const &)info point:(CGPoint)point;
- (id) initWithApiPoint:(url_scheme::ApiPoint const &)apiPoint;
- (id) initWithBookmark:(BookmarkAndCategory)bmAndCat;
- (id) initWithName:(NSString *)name andGlobalPoint:(CGPoint)point;
@end
