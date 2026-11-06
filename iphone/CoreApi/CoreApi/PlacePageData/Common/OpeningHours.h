#import <Foundation/Foundation.h>

@protocol IOpeningHoursLocalization;

NS_ASSUME_NONNULL_BEGIN

@interface WorkingDay : NSObject

@property(nonatomic, readonly) NSString * workingDays;
@property(nonatomic, readonly) NSString * workingTimes;
@property(nonatomic, readonly) BOOL isOpen;

@end

@interface OpeningHours : NSObject

/// The week starting from today's row, or a single 24/7 row.
@property(nonatomic, readonly) NSArray<WorkingDay *> * days;
@property(nonatomic, readonly) BOOL isClosedNow;

- (nullable instancetype)initWithRawString:(NSString *)rawString
                              localization:(id<IOpeningHoursLocalization>)localization;
/// @param today 1 = Sunday … 7 = Saturday, as in NSCalendar.
- (nullable instancetype)initWithRawString:(NSString *)rawString
                              localization:(id<IOpeningHoursLocalization>)localization
                                     today:(NSInteger)today;

@end

NS_ASSUME_NONNULL_END
