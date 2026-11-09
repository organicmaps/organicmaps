#import <Foundation/Foundation.h>

@protocol IOpeningHoursLocalization;

NS_ASSUME_NONNULL_BEGIN

@interface WorkingDay : NSObject

@property(nonatomic, readonly) NSString * workingDays;
@property(nonatomic, readonly) NSString * workingTimes;
@property(nonatomic, readonly) BOOL isOpen;

@end

@interface OpeningHours : NSObject

/// The week starting from today's row. Empty when open 24/7.
@property(nonatomic, readonly) NSArray<WorkingDay *> * days;

- (nullable instancetype)initWithRawString:(NSString *)rawString
                              localization:(id<IOpeningHoursLocalization>)localization;
/// For tests: lists the week from the given day instead of today.
/// @param today 1 = Sunday … 7 = Saturday, as in NSCalendar.
- (nullable instancetype)initWithRawString:(NSString *)rawString
                              localization:(id<IOpeningHoursLocalization>)localization
                                     today:(NSInteger)today;

@end

NS_ASSUME_NONNULL_END
