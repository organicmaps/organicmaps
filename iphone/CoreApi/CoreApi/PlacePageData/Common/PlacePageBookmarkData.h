#import <Foundation/Foundation.h>
#import "MWMTypes.h"
#import "MWMBookmarkColor.h"

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageBookmarkData : NSObject

@property(nonatomic, readonly) MWMMarkID bookmarkId;
@property(nonatomic, readonly) MWMMarkGroupID bookmarkGroupId;
@property(nonatomic, readonly, nullable) NSString *externalTitle;
@property(nonatomic, readonly, nullable) NSString *bookmarkDescription;
@property(nonatomic, readonly, nullable) NSString *bookmarkCategory;
@property(nonatomic, readonly) BOOL isHtmlDescription;
@property(nonatomic, readonly) MWMBookmarkColor color;

@end

NS_ASSUME_NONNULL_END
