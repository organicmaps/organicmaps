#import <UIKit/UIKit.h>
#import "MessageComposeViewController.h"
#import "MailComposeViewController.h"
#import "ColorPickerView.h"
#import "../../../map/bookmark.hpp"

typedef NS_ENUM(NSUInteger, PlacePageVCMode) {
  PlacePageVCModeEditing,
  PlacePageVCModeSaved,
};

@class PlacePageVC;
@protocol PlacePageVCDelegate <NSObject>

- (void)placePageVC:(PlacePageVC *)placePageVC didUpdateBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory;

@end

namespace search { struct AddressInfo; }
namespace url_scheme { struct ApiPoint; }

@interface PlacePageVC : UITableViewController <UITextFieldDelegate, UITextViewDelegate, UIGestureRecognizerDelegate, ColorPickerDelegate>

- (id)initWithInfo:(search::AddressInfo const &)info point:(CGPoint)point;
- (id)initWithApiPoint:(url_scheme::ApiPoint const &)apiPoint;
- (id)initWithBookmark:(BookmarkAndCategory)bmAndCat;
- (id)initWithName:(NSString *)name andGlobalPoint:(CGPoint)point;

@property (nonatomic, weak) id <PlacePageVCDelegate> delegate;
@property (nonatomic) PlacePageVCMode mode;

@end
