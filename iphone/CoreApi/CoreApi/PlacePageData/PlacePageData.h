#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

#import "MWMTypes.h"

@class PlacePageButtonsData;
@class PlacePagePreviewData;
@class PlacePageInfoData;
@class PlacePageBookmarkData;
@class CatalogPromoData;
@class HotelBookingData;
@class HotelRooms;
@class UgcData;
@class ElevationProfileData;
@class MWMMapNodeAttributes;

typedef NS_ENUM(NSInteger, PlacePageSponsoredType) {
  PlacePageSponsoredTypeNone,
  PlacePageSponsoredTypeBooking,
  PlacePageSponsoredTypeOpentable,
  PlacePageSponsoredTypePartner,
  PlacePageSponsoredTypeHoliday,
  PlacePageSponsoredTypePromoCatalogCity,
  PlacePageSponsoredTypePromoCatalogSightseeings,
  PlacePageSponsoredTypePromoCatalogOutdoor
};

typedef NS_ENUM(NSInteger, PlacePageTaxiProvider) {
  PlacePageTaxiProviderNone,
  PlacePageTaxiProviderUber,
  PlacePageTaxiProviderYandex,
  PlacePageTaxiProviderMaxim,
  PlacePageTaxiProviderRutaxi,
  PlacePageTaxiProviderFreenow,
  PlacePageTaxiProviderYango,
  PlacePageTaxiProviderCitymobil,
};

typedef NS_ENUM(NSInteger, PlacePageRoadType) {
  PlacePageRoadTypeToll,
  PlacePageRoadTypeFerry,
  PlacePageRoadTypeDirty,
  PlacePageRoadTypeNone
};

@protocol IOpeningHoursLocalization;

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageData : NSObject

@property(nonatomic, readonly, class) BOOL isGuide;
@property(class, nonatomic, readonly) BOOL hasData;

@property(nonatomic, readonly, nullable) PlacePageButtonsData *buttonsData;
@property(nonatomic, readonly) PlacePagePreviewData *previewData;
@property(nonatomic, readonly, nullable) PlacePageInfoData *infoData;
@property(nonatomic, readonly, nullable) PlacePageBookmarkData *bookmarkData;
@property(nonatomic, readonly) PlacePageSponsoredType sponsoredType;
@property(nonatomic, readonly) PlacePageTaxiProvider taxiProvider;
@property(nonatomic, readonly) PlacePageRoadType roadType;
@property(nonatomic, readonly, nullable) NSString *wikiDescriptionHtml;
@property(nonatomic, readonly, nullable) CatalogPromoData *catalogPromo;
@property(nonatomic, readonly, nullable) HotelBookingData *hotelBooking;
@property(nonatomic, readonly, nullable) HotelRooms *hotelRooms;
@property(nonatomic, readonly, nullable) UgcData *ugcData;
@property(nonatomic, readonly, nullable) ElevationProfileData *elevationProfileData;
@property(nonatomic, readonly, nullable) MWMMapNodeAttributes *mapNodeAttributes;
@property(nonatomic, readonly, nullable) NSString *bookingSearchUrl;
@property(nonatomic, readonly) BOOL isLargeToponim;
@property(nonatomic, readonly) BOOL isSightseeing;
@property(nonatomic, readonly) BOOL isOutdoor;
@property(nonatomic, readonly) BOOL isPromoCatalog;
@property(nonatomic, readonly) BOOL isPartner;
@property(nonatomic, readonly) BOOL shouldShowUgc;
@property(nonatomic, readonly) BOOL isMyPosition;
@property(nonatomic, readonly) BOOL isPreviewPlus;
@property(nonatomic, readonly) BOOL isRoutePoint;
@property(nonatomic, readonly) NSInteger partnerIndex;
@property(nonatomic, readonly, nullable) NSString *partnerName;
@property(nonatomic, readonly) CLLocationCoordinate2D locationCoordinate;
@property(nonatomic, readonly) NSArray<NSString *> *ratingCategories;
@property(nonatomic, readonly) NSString *statisticsTags;
@property(nonatomic, readonly, nullable) NSString *sponsoredURL;
@property(nonatomic, readonly, nullable) NSString *sponsoredDescriptionURL;
@property(nonatomic, readonly, nullable) NSString *sponsoredMoreURL;
@property(nonatomic, readonly, nullable) NSString *sponsoredReviewURL;
@property(nonatomic, readonly, nullable) NSString *sponsoredDeeplink;
@property(nonatomic, copy, nullable) MWMVoidBlock onBookmarkStatusUpdate;
@property(nonatomic, copy, nullable) MWMVoidBlock onMapNodeStatusUpdate;
@property(nonatomic, copy, nullable) MWMVoidBlock onUgcStatusUpdate;
@property(nonatomic, copy, nullable) void (^onMapNodeProgressUpdate)(uint64_t downloadedBytes, uint64_t totalBytes);

- (instancetype)initWithLocalizationProvider:(id<IOpeningHoursLocalization>)localization;
- (instancetype)init NS_UNAVAILABLE;

- (void)loadOnlineDataWithCompletion:(MWMVoidBlock)completion;
- (void)loadUgcWithCompletion:(MWMVoidBlock)completion;
- (void)loadCatalogPromoWithCompletion:(MWMVoidBlock)completion;
- (void)updateBookmarkStatus;
- (void)updateUgcStatus;

@end

NS_ASSUME_NONNULL_END
