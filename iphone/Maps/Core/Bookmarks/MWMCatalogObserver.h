#import "MWMCatalogCommon.h"

#include "map/bookmark_catalog.hpp"

#include "platform/remote_file.hpp"

@interface MWMCatalogObserver : NSObject

@property (copy, nonatomic) NSString * categoryId;
@property (copy, nonatomic) ProgressBlock progressBlock;
@property (copy, nonatomic) CompletionBlock completionBlock;

- (void)onDownloadStart;
- (void)onDownloadComplete:(BookmarkCatalog::DownloadResult)result;
- (void)onImportStart;
- (void)onImportCompleteSuccessful:(BOOL)success forCategoryId:(UInt64)categoryId;

@end
