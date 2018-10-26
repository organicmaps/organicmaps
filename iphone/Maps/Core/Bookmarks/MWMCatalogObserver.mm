#import "MWMCatalogObserver.h"

@implementation MWMCatalogObserver

- (void)onDownloadStart
{
  if (self.progressBlock)
    self.progressBlock(MWMCategoryProgressDownloadStarted);
}

- (void)onDownloadComplete:(BookmarkCatalog::DownloadResult)result
{
  MWMCategoryDownloadStatus downloadStatus;
  switch (result)
  {
    case BookmarkCatalog::DownloadResult::Success:
      if (self.progressBlock)
        self.progressBlock(MWMCategoryProgressDownloadFinished);
      return;
    case BookmarkCatalog::DownloadResult::AuthError:
      downloadStatus = MWMCategoryDownloadStatusForbidden;
      break;
    case BookmarkCatalog::DownloadResult::ServerError:
      downloadStatus = MWMCategoryDownloadStatusNotFound;
      break;
    case BookmarkCatalog::DownloadResult::NetworkError:
      downloadStatus = MWMCategoryDownloadStatusNetworkError;
      break;
    case BookmarkCatalog::DownloadResult::DiskError:
      downloadStatus = MWMCategoryDownloadStatusDiskError;
      break;
    case BookmarkCatalog::DownloadResult::NeedPayment:
      //TODO(@beloal)
      break;
  }
  if (self.completionBlock)
    self.completionBlock(0, [[NSError alloc] initWithDomain:kCatalogErrorDomain
                                                       code:kCategoryDownloadFailedCode
                                                   userInfo:@{kCategoryDownloadStatusKey : @(downloadStatus)}]);
}

- (void)onImportStart
{
  if (self.progressBlock)
    self.progressBlock(MWMCategoryProgressImportStarted);
}

- (void)onImportCompleteSuccessful:(BOOL)success forCategoryId:(UInt64)categoryId
{
  if (self.completionBlock) {
    NSError * error = success ? nil : [[NSError alloc] initWithDomain:kCatalogErrorDomain
                                                                 code:kCategoryImportFailedCode
                                                             userInfo:nil];
    self.completionBlock(categoryId, error);
  }
}

@end
