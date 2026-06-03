#import <Foundation/Foundation.h>

#import "MWMTypes.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, BookmarksCategoryLoadingSource) {
  BookmarksCategoryLoadingSourceInitial,
  BookmarksCategoryLoadingSourceImport,
  BookmarksCategoryLoadingSourceReload
};

@interface BookmarksCategoryLoadingResult : NSObject

@property(nonatomic, readonly) BookmarksCategoryLoadingSource source;
@property(nonatomic, readonly, copy) MWMGroupIDCollection groupIds;
@property(nonatomic, readonly) BOOL success;
@property(nonatomic, readonly, nullable, copy) NSURL * fileURL;
@property(nonatomic, readonly) BOOL temporaryFile;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
