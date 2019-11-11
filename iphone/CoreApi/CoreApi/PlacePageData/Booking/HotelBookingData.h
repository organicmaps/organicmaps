#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface HotelFacility : NSObject

@property(nonatomic, readonly) NSString *type;
@property(nonatomic, readonly) NSString *name;

@end

@interface HotelPhotoUrl : NSObject

@property(nonatomic, readonly) NSString *original;
@property(nonatomic, readonly) NSString *thumbnail;

@end

@interface HotelReview : NSObject

@property(nonatomic, readonly) NSDate *date;
@property(nonatomic, readonly) float score;
@property(nonatomic, readonly) NSString *author;
@property(nonatomic, readonly, nullable) NSString *pros;
@property(nonatomic, readonly, nullable) NSString *cons;

@end

@interface HotelBookingData : NSObject

@property(nonatomic, readonly) NSString *hotelId;
@property(nonatomic, readonly) NSString *hotelDescription;
@property(nonatomic, readonly) float score;
@property(nonatomic, readonly) NSUInteger scoreCount;
@property(nonatomic, readonly) NSArray<HotelFacility *> *facilities;
@property(nonatomic, readonly) NSArray<HotelPhotoUrl *> *photos;
@property(nonatomic, readonly) NSArray<HotelReview *> *reviews;

@end

NS_ASSUME_NONNULL_END
