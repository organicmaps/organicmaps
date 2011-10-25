#include "download_manager.hpp"

#include "../std/target_os.hpp"

#include "../base/logging.hpp"

#import "apple_download.h"
#ifdef OMIM_OS_IPHONE
  #import "../iphone/Maps/Classes/MapsAppDelegate.h"
  #import <UIKit/UIApplication.h>
#endif

class AppleDownloadManager : public DownloadManager
{
  NSMutableArray * activeDownloads;

public:
  AppleDownloadManager()
  {
    activeDownloads = [[NSMutableArray alloc] init];
  }

  virtual ~AppleDownloadManager()
  {
    for (NSUInteger i = 0; i < [activeDownloads count]; ++i)
      [[activeDownloads objectAtIndex:i] release];

    [activeDownloads removeAllObjects];
    [activeDownloads release];
  }

  virtual void HttpRequest(HttpStartParams const & params)
  {
    // check if download is already active
    for (NSUInteger i = 0; i < [activeDownloads count]; ++i)
    {
      AppleDownload * download = [activeDownloads objectAtIndex:i];
      if ([download Url] == params.m_url)
      {
        LOG(LWARNING, ("Download is already active for url %s", params.m_url));
        return;
      }
    }

    AppleDownload * download = [[AppleDownload alloc] init];
    if ([download StartDownload:params])
    {
      // save download in array to cancel it later if necessary
      [activeDownloads addObject:download];
#ifdef OMIM_OS_IPHONE
      // prevent device from going to standby
      [[MapsAppDelegate theApp] disableStandby];
#endif
    }
    else
    {
      // free memory
      [download release];
    }
  }

  /// @note Doesn't notify clients on canceling!
  virtual void CancelDownload(string const & url)
  {
#ifdef OMIM_OS_IPHONE
    // disable network activity indicator in top system toolbar
    // note that this method is called also from successful/failed download to "selfdestruct" below
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
#endif

    for (NSUInteger i = 0; i < [activeDownloads count]; ++i)
    {
      AppleDownload * download = [activeDownloads objectAtIndex:i];
      if ([download Url] == url)
      {
        [activeDownloads removeObjectAtIndex:i];
        [download Cancel];
        [download release];
        break;
      }
    }

#ifdef OMIM_OS_IPHONE
    // enable standby if no more downloads are left
    if ([activeDownloads count] == 0)
      [[MapsAppDelegate theApp] enableStandby];
#endif
  }

  virtual void CancelAllDownloads()
  {
#ifdef OMIM_OS_IPHONE
    // disable network activity indicator in top system toolbar
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
#endif

    for (NSUInteger i = 0; i < [activeDownloads count]; ++i) {
      AppleDownload * download = [activeDownloads objectAtIndex:i];
      [download Cancel];
      [download release];
    }
    [activeDownloads removeAllObjects];
  }
};

DownloadManager & GetDownloadManager()
{
  static AppleDownloadManager downloadManager;
  return downloadManager;
}
