#import "MWMCatalogCommon.h"

#include "platform/remote_file.hpp"

@interface MWMCatalogObserver : NSObject

@property (copy, nonatomic) NSString * categoryId;
@property (copy, nonatomic) ProgressBlock progressBlock;
@property (copy, nonatomic) CompletionBlock completionBlock;

- (void)onDownloadStart;
- (void)onDownloadComplete:(platform::RemoteFile::Status)status;
- (void)onImportStart;
- (void)onImportCompleteSuccessful:(BOOL)success;

@end
