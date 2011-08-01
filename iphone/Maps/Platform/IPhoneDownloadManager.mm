#include "../../platform/download_manager.hpp"

#import "IPhoneDownload.h"
#import "MapsAppDelegate.h"

#import <UIKit/UIApplication.h>

class IPhoneDownloadManager : public DownloadManager
{
	NSMutableArray * activeDownloads;

public:
	IPhoneDownloadManager()
  {
  	activeDownloads = [[NSMutableArray alloc] init];
  }

	virtual ~IPhoneDownloadManager()
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
    	IPhoneDownload * download = [activeDownloads objectAtIndex:i];
    	if ([download Url] == params.m_url)
      {
      	NSLog(@"Download is already active for url %s", params.m_url.c_str());
      	return;
      }
    }

  	IPhoneDownload * download = [[IPhoneDownload alloc] init];
    if ([download StartDownload:params])
    {
    	// save download in array to cancel it later if necessary
    	[activeDownloads addObject:download];
      // prevent device from going to standby
      [[MapsAppDelegate theApp] disableStandby];
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
    // disable network activity indicator in top system toolbar
    // note that this method is called also from successful/failed download to "selfdestruct" below
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;

    for (NSUInteger i = 0; i < [activeDownloads count]; ++i)
    {
    	IPhoneDownload * download = [activeDownloads objectAtIndex:i];
    	if ([download Url] == url)
      {
      	[activeDownloads removeObjectAtIndex:i];
        [download Cancel];
        [download release];
      	break;
      }
    }
    // enable standby if no more downloads are left
    if ([activeDownloads count] == 0)
      [[MapsAppDelegate theApp] enableStandby];
  }

  virtual void CancelAllDownloads()
  {
    // disable network activity indicator in top system toolbar
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;

  	for (NSUInteger i = 0; i < [activeDownloads count]; ++i) {
    	IPhoneDownload * download = [activeDownloads objectAtIndex:i];
      [download Cancel];
      [download release];
    }
    [activeDownloads removeAllObjects];
  }
};

DownloadManager & GetDownloadManager()
{
  static IPhoneDownloadManager downloadManager;
  return downloadManager;
}
