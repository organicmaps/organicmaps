#import <Foundation/Foundation.h>

#include "../../../platform/download_manager.hpp"
#include "../../../std/string.hpp"

@interface IPhoneDownload : NSObject
{
	/// used to delete file on canceling
  /// @note DOWNLOADING_FILE_EXTENSION should be appended to get real downloading file name
	string m_requestedFileName;
	FILE * m_file;
  /// stores information from the server, can be zero
  int64_t m_projectedFileSize;
	string m_url;
  NSURLConnection * m_connection;

  TDownloadFinishedFunction m_finishObserver;
  TDownloadProgressFunction m_progressObserver;

  NSInteger m_retryCounter;
}

- (void) dealloc;
- (std::string const &) Url;
- (BOOL) StartDownloadWithUrl: (char const *)originalUrl andFile: (char const *)file
		andFinishFunc: (TDownloadFinishedFunction &)finishFunc
    andProgressFunc: (TDownloadProgressFunction &)progressFunc
    andUseResume: (BOOL)resume;
// Added because release from manager doesn't destroy it immediately...
- (void) Cancel;
@end
