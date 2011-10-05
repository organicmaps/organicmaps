#import <Foundation/Foundation.h>

#include "../../../platform/download_manager.hpp"
#include "../../../platform/url_generator.hpp"
#include "../../../std/string.hpp"

@interface IPhoneDownload : NSObject
{
  HttpStartParams m_params;
  
  string m_currentUrl;
  UrlGenerator m_urlGenerator;

	FILE * m_file;
  /// stores information from the server, can be zero
  int64_t m_projectedFileSize;
  NSURLConnection * m_connection;

  string m_receivedBuffer;
}

- (void) dealloc;
- (std::string const &) Url;
- (BOOL) StartDownload: (HttpStartParams const &)params;
// Added because release from manager doesn't destroy it immediately...
- (void) Cancel;
@end
