#import "MWMCatalogCommon.h"

#include "map/bookmark_catalog.hpp"

@interface MWMCatalogObserver : NSObject

@property (copy, nonatomic) NSString * categoryId;
@property (copy, nonatomic) ProgressBlock progressBlock;
@property (copy, nonatomic) DownloadCompletionBlock downloadCompletionBlock;
@property (copy, nonatomic) UploadCompletionBlock uploadCompletionBlock;

- (void)onDownloadStart;
- (void)onDownloadComplete:(BookmarkCatalog::DownloadResult)result;
- (void)onImportStart;
- (void)onImportCompleteSuccessful:(BOOL)success forCategoryId:(UInt64)categoryId;
- (void)onUploadStart;
- (void)onUploadComplete:(BookmarkCatalog::UploadResult)result withUrl:(NSURL *)categoryUrl;

@end
