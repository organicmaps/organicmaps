
#import <UIKit/UIKit.h>
#include "../../map/bookmark.hpp"
#include "../../geometry/point2d.hpp"
#import "UIKitCategories.h"
#import "LocationManager.h"

typedef NS_ENUM(NSUInteger, PlacePageState) {
  PlacePageStateHidden,
  PlacePageStatePreview,
  PlacePageStateOpened,
};

@class PlacePageView;
@protocol PlacePageViewDelegate <NSObject>

- (void)placePageView:(PlacePageView *)placePage willEditProperty:(NSString *)propertyName inBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory;

- (void)placePageView:(PlacePageView *)placePage willShareText:(NSString *)text point:(m2::PointD)point;
- (void)placePageView:(PlacePageView *)placePage willShareApiPoint:(ApiMarkPoint const *)point;

- (void)placePageViewDidStartRouting:(PlacePageView *)placePage;

- (void)placePageViewWillEnterState:(PlacePageState)state;

@end


@interface PlacePageView : SolidTouchView <LocationObserver>

- (void)showUserMark:(unique_ptr<UserMarkCopy> &&)mark;

- (void)setState:(PlacePageState)state animated:(BOOL)animated withCallback:(BOOL)withCallback;
@property (nonatomic, readonly) PlacePageState state;

@property (nonatomic, weak) id <PlacePageViewDelegate> delegate;

- (m2::PointD)pinPoint;
- (BOOL)isMyPosition;
@property (nonatomic, readonly) NSString * temporaryTitle;
@property (nonatomic) BOOL statusBarIncluded;

- (void)showBuildingRoutingActivity:(BOOL)show;

@end
