#import <UIKit/UIKit.h>


namespace search { struct AddressInfo; }
namespace url_scheme { struct ApiPoint; }

@interface PlacePreviewViewController : UITableViewController <UIGestureRecognizerDelegate>

- (id)initWith:(search::AddressInfo const &)info point:(CGPoint)point;
- (id)initWithApiPoint:(url_scheme::ApiPoint const &)apiPoint;
- (id)initWithPoint:(CGPoint)point;

@end
