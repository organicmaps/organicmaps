#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, PlacePageDataLocalAdsStatus) {
  PlacePageDataLocalAdsStatusNotAvailable,
  PlacePageDataLocalAdsStatusCandidate,
  PlacePageDataLocalAdsStatusCustomer,
  PlacePageDataLocalAdsStatusHidden
};

@class OpeningHours;

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageInfoData : NSObject

@property(nonatomic, readonly, nullable) NSString *openingHoursString;
@property(nonatomic, readonly, nullable) OpeningHours *openingHours;
@property(nonatomic, readonly, nullable) NSString *phone;
@property(nonatomic, readonly, nullable) NSURL *phoneUrl;
@property(nonatomic, readonly, nullable) NSString *website;
@property(nonatomic, readonly, nullable) NSString *email;
@property(nonatomic, readonly, nullable) NSString *cuisine;
@property(nonatomic, readonly, nullable) NSString *ppOperator;
@property(nonatomic, readonly, nullable) NSString *address;
@property(nonatomic, readonly, nullable) NSString *rawCoordinates;
@property(nonatomic, readonly, nullable) NSString *formattedCoordinates;
@property(nonatomic, readonly, nullable) NSString *localAdsUrl;
@property(nonatomic, readonly) BOOL wifiAvailable;
@property(nonatomic, readonly) PlacePageDataLocalAdsStatus localAdsStatus;

@end

NS_ASSUME_NONNULL_END
