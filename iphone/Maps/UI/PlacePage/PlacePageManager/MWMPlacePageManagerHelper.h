#import "MWMUGCReviewSource.h"
#import <CoreApi/UgcSummaryRatingType.h>

@class PlacePageData;
@class ElevationProfileData;

@interface MWMPlacePageManagerHelper : NSObject

+ (void)updateAvailableArea:(CGRect)frame;
+ (void)showUGCAddReview:(PlacePageData *)data
                  rating:(UgcSummaryRatingType)value
              fromSource:(MWMUGCReviewSource)source;
+ (void)searchSimilar:(PlacePageData *)data;
+ (void)editPlace;
+ (void)addBusiness;
+ (void)addPlace:(CLLocationCoordinate2D)coordinate;
+ (void)orderTaxi:(PlacePageData *)data;
+ (void)taxiShown:(PlacePageData *)data;
+ (void)openLocalAdsURL:(PlacePageData *)data;
+ (void)openWebsite:(PlacePageData *)data;
+ (void)call:(PlacePageData *)data;
+ (void)showAllFacilities:(PlacePageData *)data;
+ (void)showPlaceDescription:(NSString *)htmlString;
+ (void)openMoreUrl:(PlacePageData *)data;
+ (void)openReviewUrl:(PlacePageData *)data;
+ (void)openDescriptionUrl:(PlacePageData *)data;
+ (void)openCatalogSingleItem:(PlacePageData *)data atIndex:(NSInteger)index;
+ (void)openCatalogMoreItems:(PlacePageData *)data;
+ (void)addBookmark:(PlacePageData *)data;
+ (void)removeBookmark:(PlacePageData *)data;
+ (void)editBookmark:(PlacePageData *)data;
+ (void)searchBookingHotels:(PlacePageData *)data;
+ (void)openPartner:(PlacePageData *)data;
+ (void)book:(PlacePageData *)data;
+ (void)share:(PlacePageData *)data;
+ (void)routeFrom:(PlacePageData *)data;
+ (void)routeTo:(PlacePageData *)data;
+ (void)routeAddStop:(PlacePageData *)data;
+ (void)routeRemoveStop:(PlacePageData *)data;
+ (void)avoidDirty;
+ (void)avoidFerry;
+ (void)avoidToll;
+ (void)showRemoveAds;
+ (void)openElevationDifficultPopup:(PlacePageData *)data;

@end
