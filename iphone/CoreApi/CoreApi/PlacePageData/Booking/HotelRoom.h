#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, HotelRoomDealType) {
  HotelRoomDealTypeSmart,
  HotelRoomDealTypeLastMinute
};

NS_ASSUME_NONNULL_BEGIN

@interface HotelRoomDeal : NSObject

@property(nonatomic, readonly) NSUInteger discount;
@property(nonatomic, readonly) NSArray<NSNumber *> *types;

@end

@interface HotelRoom : NSObject

@property(nonatomic, readonly) NSString *offerId;
@property(nonatomic, readonly) NSString *name;
@property(nonatomic, readonly) NSString *roomDescription;
@property(nonatomic, readonly) NSUInteger maxOccupancy;
@property(nonatomic, readonly) double price;
@property(nonatomic, readonly) NSString *currency;
@property(nonatomic, readonly) NSArray<NSString *> *photos;
@property(nonatomic, readonly, nullable) HotelRoomDeal *deal;
@property(nonatomic, readonly, nullable) NSDate *refundableUntil;
@property(nonatomic, readonly) BOOL isBreakfastIncluded;
@property(nonatomic, readonly) BOOL isDepositRequired;
@property(nonatomic, readonly) NSUInteger discount;

@end

NS_ASSUME_NONNULL_END
