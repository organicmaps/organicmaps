#import <Foundation/Foundation.h>

@class HotelRoom;

NS_ASSUME_NONNULL_BEGIN

@interface HotelRooms : NSObject

@property(nonatomic, readonly) double minPrice;
@property(nonatomic, readonly) NSString *currency;
@property(nonatomic, readonly) NSUInteger discount;
@property(nonatomic, readonly) BOOL isSmartDeal;
@property(nonatomic, readonly) NSArray<HotelRoom *> *rooms;

@end

NS_ASSUME_NONNULL_END
