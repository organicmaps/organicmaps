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
  PlacePageTaxiProviderRutaxi
};

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageData : NSObject

@property(nonatomic, readonly, nullable) PlacePageButtonsData *buttonsData;
@property(nonatomic, readonly) PlacePagePreviewData *previewData;
@property(nonatomic, readonly) PlacePageInfoData *infoData;
@property(nonatomic, readonly, nullable) PlacePageBookmarkData *bookmarkData;
@property(nonatomic, readonly) PlacePageSponsoredType sponsoredType;
@property(nonatomic, readonly) PlacePageTaxiProvider taxiProvider;
@property(nonatomic, readonly, nullable) NSString *wikiDescriptionHtml;
@property(nonatomic, readonly, nullable) CatalogPromoData *catalogPromo;
@property(nonatomic, readonly, nullable) HotelBookingData *hotelBooking;
@property(nonatomic, readonly, nullable) HotelRooms *hotelRooms;
@property(nonatomic, readonly, nullable) UgcData *ugcData;
@property(nonatomic, readonly, nullable) NSString *bookingSearchUrl;
@property(nonatomic, readonly) BOOL isLargeToponim;
@property(nonatomic, readonly) BOOL isSightseeing;
@property(nonatomic, readonly) BOOL isPromoCatalog;
@property(nonatomic, readonly) BOOL shouldShowUgc;
@property(nonatomic, readonly) BOOL isMyPosition;
@property(nonatomic, readonly) CLLocationCoordinate2D locationCoordinate;

- (void)loadOnlineDataWithCompletion:(MWMVoidBlock)completion;
- (void)loadUgcWithCompletion:(MWMVoidBlock)completion;
- (void)loadCatalogPromoWithCompletion:(MWMVoidBlock)completion;

@end

NS_ASSUME_NONNULL_END
