#import <Foundation/Foundation.h>

#import "MWMCatalogCommon.h"
#include "platform/remote_file.hpp"

@interface MWMCatalogObserver : NSObject
@property (copy, nonatomic) NSString * categoryId;
@property (copy, nonatomic) void(^progressBlock)(MWMCategoryProgress progress);
@property (copy, nonatomic) void(^completionBlock)(NSError * error);
- (void)onDownloadStart;
- (void)onDownloadComplete:(platform::RemoteFile::Status)status;
- (void)onImportStart;
- (void)onImportCompleteSuccessful:(BOOL)success;
@end
