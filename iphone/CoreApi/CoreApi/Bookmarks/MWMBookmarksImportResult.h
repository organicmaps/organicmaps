#import <Foundation/Foundation.h>

#import "MWMTypes.h"

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(BookmarksImportResult)
@interface MWMBookmarksImportResult : NSObject

@property(nonatomic, readonly, copy) MWMGroupIDCollection groupIds;
@property(nonatomic, readonly, copy) NSArray<NSString *> * failedFileNames;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
