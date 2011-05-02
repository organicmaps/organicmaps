#import <Foundation/Foundation.h>

#include "../../../platform/download_manager.hpp"
#include "../../../std/string.hpp"

@interface IPhoneDownload : NSObject
{
  HttpStartParams m_params;

	FILE * m_file;
  /// stores information from the server, can be zero
  int64_t m_projectedFileSize;
  NSURLConnection * m_connection;

  NSInteger m_retryCounter;

  string m_receivedBuffer;
}

- (void) dealloc;
- (std::string const &) Url;
- (BOOL) StartDownload: (HttpStartParams const &)params;
// Added because release from manager doesn't destroy it immediately...
- (void) Cancel;
@end
