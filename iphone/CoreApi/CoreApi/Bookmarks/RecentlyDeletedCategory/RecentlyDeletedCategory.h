#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface RecentlyDeletedCategory : NSObject

@property(nonatomic, readonly) NSString * title;
@property(nonatomic, readonly) NSURL * fileURL;
@property(nonatomic, readonly) NSDate * deletionDate;

- (instancetype)initTitle:(NSString *)title fileURL:(NSURL *)fileURL deletionDate:(NSDate *)deletionDate;

@end

NS_ASSUME_NONNULL_END
