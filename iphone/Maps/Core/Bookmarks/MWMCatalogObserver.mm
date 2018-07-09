#import "MWMCatalogObserver.h"

@implementation MWMCatalogObserver

- (void)onDownloadStart
{
  if (self.progressBlock)
    self.progressBlock(MWMCategoryProgressDownloadStarted);
}

- (void)onDownloadComplete:(platform::RemoteFile::Status)status
{
  MWMCategoryDownloadStatus downloadStatus;
  switch (status)
  {
    case platform::RemoteFile::Status::Ok:
      if (self.progressBlock)
        self.progressBlock(MWMCategoryProgressDownloadFinished);
      return;
    case platform::RemoteFile::Status::Forbidden:
      downloadStatus = MWMCategoryDownloadStatusForbidden;
      break;
    case platform::RemoteFile::Status::NotFound:
      downloadStatus = MWMCategoryDownloadStatusNotFound;
      break;
    case platform::RemoteFile::Status::NetworkError:
      downloadStatus = MWMCategoryDownloadStatusNetworkError;
      break;
    case platform::RemoteFile::Status::DiskError:
      downloadStatus = MWMCategoryDownloadStatusDiskError;
      break;
  }
  if (self.completionBlock)
    self.completionBlock([[NSError alloc] initWithDomain:kCatalogErrorDomain
                                                    code:kCategoryDownloadFailedCode
                                                userInfo:@{kCategoryDownloadStatusKey : @(downloadStatus)}]);
}

- (void)onImportStart
{
  if (self.progressBlock)
    self.progressBlock(MWMCategoryProgressImportStarted);
}

- (void)onImportCompleteSuccessful:(BOOL)success
{
  if (self.completionBlock) {
    NSError * error = success ? nil : [[NSError alloc] initWithDomain:kCatalogErrorDomain
                                                                 code:kCategoryImportFailedCode
                                                             userInfo:nil];
    self.completionBlock(error);
  }
}

@end
