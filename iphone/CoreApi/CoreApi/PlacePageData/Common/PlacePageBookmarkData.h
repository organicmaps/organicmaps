#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageBookmarkData : NSObject

@property(nonatomic, readonly) uint64_t bookmarkId;
@property(nonatomic, readonly) uint64_t bookmarkGroupId;
@property(nonatomic, readonly, nullable) NSString *externalTitle;
@property(nonatomic, readonly, nullable) NSString *bookmarkDescription;
@property(nonatomic, readonly, nullable) NSString *bookmarkCategory;
@property(nonatomic, readonly) BOOL isHtmlDescription;
@property(nonatomic, readonly) BOOL isEditable;

@end

NS_ASSUME_NONNULL_END
