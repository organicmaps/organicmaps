
#import <UIKit/UIKit.h>
#include "../../map/bookmark.hpp"
#include "../../geometry/point2d.hpp"

namespace search { struct AddressInfo; }
namespace url_scheme { struct ApiPoint; }

typedef NS_ENUM(NSUInteger, PlacePageState) {
  PlacePageStateHidden,
  PlacePageStatePreview,
  PlacePageStateOpened,
};

@class PlacePageView;
@protocol PlacePageViewDelegate <NSObject>

- (void)placePageView:(PlacePageView *)placePage willEditBookmarkWithInfo:(search::AddressInfo const &)addressInfo point:(const m2::PointD &)point;
- (void)placePageView:(PlacePageView *)placePage willEditProperty:(NSString *)propertyName inBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory;

- (void)placePageView:(PlacePageView *)placePage willShareInfo:(search::AddressInfo const &)addressInfo point:(const m2::PointD &)point;
- (void)placePageView:(PlacePageView *)placePage willShareApiPoint:(const url_scheme::ApiPoint &)point;

@end

@interface PlacePageView : UIView

- (void)showUserMark:(UserMark const *)mark;

- (void)setState:(PlacePageState)state animated:(BOOL)animated withCallback:(BOOL)withCallback;
@property (nonatomic, readonly) PlacePageState state;

@property (nonatomic, weak) id <PlacePageViewDelegate> delegate;

@property (nonatomic, readonly) m2::PointD pinPoint;

@property (nonatomic) BOOL statusBarIncluded;

@end
