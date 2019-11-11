#import <Foundation/Foundation.h>

@class UgcSummaryRating;

NS_ASSUME_NONNULL_BEGIN

@interface UgcStarRating : NSObject

@property(nonatomic, readonly) NSString *title;
@property(nonatomic, readonly) float value;

@end

@interface UgcReview : NSObject

@property(nonatomic, readonly) uint64_t reviewId;
@property(nonatomic, readonly) NSString *author;
@property(nonatomic, readonly) NSString *text;
@property(nonatomic, readonly) float rating;
@property(nonatomic, readonly) NSDate *date;

@end

@interface UgcMyReview : NSObject

@property(nonatomic, readonly) NSString *text;
@property(nonatomic, readonly) NSArray<UgcStarRating *> *starRatings;
@property(nonatomic, readonly) NSDate *date;

@end

@interface UgcData : NSObject

@property(nonatomic, readonly) BOOL isEmpty;
@property(nonatomic, readonly) BOOL isUpdateEmpty;
@property(nonatomic, readonly) BOOL isTotalRatingEmpty;
@property(nonatomic, readonly) NSUInteger ratingsCount;
@property(nonatomic, readonly, nullable) UgcSummaryRating *summaryRating;
@property(nonatomic, readonly) NSArray<UgcStarRating *> *starRatings;
@property(nonatomic, readonly) NSArray<UgcReview *> *reviews;
@property(nonatomic, readonly, nullable) UgcMyReview *myReview;

@end

NS_ASSUME_NONNULL_END
