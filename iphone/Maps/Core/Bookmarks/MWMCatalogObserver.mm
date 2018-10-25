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
      downloadStatus = MWMCategoryDownloadStatusNeedAuth;
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
      downloadStatus = MWMCategoryDownloadStatusNeedPayment;
      break;
  }
  if (self.downloadCompletionBlock)
    self.downloadCompletionBlock(0, [[NSError alloc] initWithDomain:kCatalogErrorDomain
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
  if (self.downloadCompletionBlock) {
    NSError * error = success ? nil : [[NSError alloc] initWithDomain:kCatalogErrorDomain
                                                                 code:kCategoryImportFailedCode
                                                             userInfo:nil];
    self.downloadCompletionBlock(categoryId, error);
  }
}

- (void)onUploadStart
{
  if (self.progressBlock)
    self.progressBlock(MWMCategoryProgressUploadStarted);
}

- (void)onUploadComplete:(BookmarkCatalog::UploadResult)result withUrl:(NSURL *)categoryUrl
{
  MWMCategoryUploadStatus uploadStatus;
  switch (result)
  {
    case BookmarkCatalog::UploadResult::Success:
      if (self.uploadCompletionBlock)
        self.uploadCompletionBlock(categoryUrl, nil);
      return;
    case BookmarkCatalog::UploadResult::NetworkError:
      uploadStatus = MWMCategoryUploadStatusNetworkError;
      break;
    case BookmarkCatalog::UploadResult::ServerError:
      uploadStatus = MWMCategoryUploadStatusServerError;
      break;
    case BookmarkCatalog::UploadResult::AuthError:
      uploadStatus = MWMCategoryUploadStatusAuthError;
      break;
    case BookmarkCatalog::UploadResult::MalformedDataError:
      uploadStatus = MWMCategoryUploadStatusMalformedData;
      break;
    case BookmarkCatalog::UploadResult::AccessError:
      uploadStatus = MWMCategoryUploadStatusAccessError;
      break;
    case BookmarkCatalog::UploadResult::InvalidCall:
      uploadStatus = MWMCategoryUploadStatusInvalidCall;
      break;
  }
  if (self.uploadCompletionBlock)
    self.uploadCompletionBlock(nil, [[NSError alloc] initWithDomain:kCatalogErrorDomain
                                                               code:kCategoryUploadFailedCode
                                                           userInfo:@{kCategoryUploadStatusKey : @(uploadStatus)}]);
}

@end
