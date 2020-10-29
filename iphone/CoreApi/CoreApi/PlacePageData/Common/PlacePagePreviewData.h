#import <Foundation/Foundation.h>

@class PlacePageScheduleData;
@class UgcSummaryRating;
@class CoreBanner;

typedef NS_ENUM(NSInteger, PlacePageDataHotelType) {
  PlacePageDataHotelTypeHotel,
  PlacePageDataHotelTypeApartment,
  PlacePageDataHotelTypeCampSite,
  PlacePageDataHotelTypeChalet,
  PlacePageDataHotelTypeGuestHouse,
  PlacePageDataHotelTypeHostel,
  PlacePageDataHotelTypeMotel,
  PlacePageDataHotelTypeResort,
  PlacePageDataHotelTypeNone
};

typedef NS_ENUM(NSInteger, PlacePageDataSchedule) {
  PlacePageDataOpeningHoursAllDay,
  PlacePageDataOpeningHoursOpen,
  PlacePageDataOpeningHoursClosed,
  PlacePageDataOpeningHoursUnknown
};

NS_ASSUME_NONNULL_BEGIN

@interface PlacePagePreviewData : NSObject

@property(nonatomic, readonly, nullable) NSString *title;
@property(nonatomic, readonly, nullable) NSString *subtitle;
@property(nonatomic, readonly, nullable) NSString *coordinates;
@property(nonatomic, readonly, nullable) NSString *address;
@property(nonatomic, readonly, nullable) NSString *pricing;
@property(nonatomic, readonly, nullable) NSNumber *rawPricing;
@property(nonatomic, readonly) float rawRating;
@property(nonatomic, readonly) PlacePageDataSchedule schedule;
@property(nonatomic, readonly) PlacePageDataHotelType hotelType;
@property(nonatomic, readonly) BOOL isMyPosition;
@property(nonatomic, readonly) BOOL hasBanner;
@property(nonatomic, readonly) BOOL isPopular;
@property(nonatomic, readonly) BOOL isTopChoice;
@property(nonatomic, readonly) BOOL isBookingPlace;
@property(nonatomic, readonly) BOOL showUgc;
@property(nonatomic, readonly, nullable) NSArray<CoreBanner *> *banners;

@end

NS_ASSUME_NONNULL_END
