#import <Foundation/Foundation.h>

@protocol IOpeningHoursLocalization;

NS_ASSUME_NONNULL_BEGIN

@interface WorkingDay : NSObject

@property(nonatomic, readonly) NSString *workingDays;
@property(nonatomic, readonly) NSString *workingTimes;
@property(nonatomic, readonly, nullable) NSString *breaks;
@property(nonatomic, readonly) BOOL isOpen;

@end

@interface OpeningHours : NSObject

@property(nonatomic, readonly) NSArray<WorkingDay *> *days;
@property(nonatomic, readonly) BOOL isClosedNow;

- (nullable instancetype)initWithRawString:(NSString *)rawString
                              localization:(id<IOpeningHoursLocalization>)localization;

@end

NS_ASSUME_NONNULL_END
