
#import <UIKit/UIKit.h>
#include "../../map/bookmark.hpp"
#include "../../geometry/point2d.hpp"

namespace search { struct AddressInfo; }
namespace url_scheme { struct ApiPoint; }

typedef NS_ENUM(NSUInteger, PlacePageState) {
  PlacePageStateHidden,
  PlacePageStateBitShown,
  PlacePageStateOpened,
};

@class PlacePageView;
@protocol PlacePageViewDelegate <NSObject>

- (void)placePageView:(PlacePageView *)placePage willEditBookmarkWithInfo:(search::AddressInfo const &)addressInfo point:(const m2::PointD &)point;
- (void)placePageView:(PlacePageView *)placePage willEditBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory;
- (void)placePageView:(PlacePageView *)placePage willShareInfo:(search::AddressInfo const &)addressInfo point:(const m2::PointD &)point;

@end

@interface PlacePageView : UIView

- (void)showPoint:(m2::PointD const &)point addressInfo:(search::AddressInfo const &)addressInfo;
- (void)showBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory;
- (void)showBookmark:(Bookmark const &)bookmark;
- (void)showApiPoint:(url_scheme::ApiPoint const &)point;

- (void)setState:(PlacePageState)state animated:(BOOL)animated;
@property (nonatomic, readonly) PlacePageState state;

@property (nonatomic, weak) id <PlacePageViewDelegate> delegate;
@property (nonatomic, readonly) m2::PointD pinPoint;

@end
