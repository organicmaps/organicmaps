#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol IOpeningHoursLocalization

@property(nonatomic, readonly) NSString *closedString;
@property(nonatomic, readonly) NSString *breakString;
@property(nonatomic, readonly) NSString *twentyFourSevenString;
@property(nonatomic, readonly) NSString *allDayString;
@property(nonatomic, readonly) NSString *dailyString;
@property(nonatomic, readonly) NSString *todayString;
@property(nonatomic, readonly) NSString *dayOffString;

@end

NS_ASSUME_NONNULL_END
