#import <Foundation/Foundation.h>

#import "IOpeningHoursLocalization.h"

NS_ASSUME_NONNULL_BEGIN

@interface WorkingDay : NSObject

@property(nonatomic, readonly) NSString *workingDays;
@property(nonatomic, readonly) NSString *workingTimes;
@property(nonatomic, readonly) NSString *breaks;

@end

@interface OpeningHours : NSObject

@property(nonatomic, readonly) NSArray<WorkingDay *> *days;

- (instancetype)initWithRawString:(NSString *)rawString localization:(id<IOpeningHoursLocalization>)localization;

@end

NS_ASSUME_NONNULL_END
