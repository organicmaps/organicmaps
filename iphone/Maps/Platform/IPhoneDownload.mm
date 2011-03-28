#import "IPhoneDownload.h"

#include "../../platform/download_manager.hpp"

#include "../../coding/base64.hpp"
#include "../../coding/sha2.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <net/if.h>

#import <UIKit/UIDevice.h>
#import <UIKit/UIApplication.h>

#if !defined(IFT_ETHER)
  #define IFT_ETHER 0x6 /* Ethernet CSMACD */
#endif

#define TIMEOUT_IN_SECONDS 15.0
#define MAX_AUTOMATIC_RETRIES 3

string GetDeviceUid()
{
  NSString * uid = [[UIDevice currentDevice] uniqueIdentifier];
  return [uid UTF8String];
}

string GetMacAddress()
{
  string result;
  // get wifi mac addr
  ifaddrs * addresses = NULL;
  if (getifaddrs(&addresses) == 0 && addresses != NULL)
  {
    ifaddrs * currentAddr = addresses;
    do
    {
      if (currentAddr->ifa_addr->sa_family == AF_LINK
          && ((const struct sockaddr_dl *) currentAddr->ifa_addr)->sdl_type == IFT_ETHER)
      {
        const struct sockaddr_dl * dlAddr = (const struct sockaddr_dl *) currentAddr->ifa_addr;
        const char * base = &dlAddr->sdl_data[dlAddr->sdl_nlen];
        result.assign(base, dlAddr->sdl_alen);
        break;
      }
      currentAddr = currentAddr->ifa_next;
    }
    while (currentAddr->ifa_next);
    freeifaddrs(addresses);
  }
  return result;
}

string GetUniqueHashedId()
{
  // generate sha2 hash for mac address
  string const hash = sha2::digest256(GetMacAddress() + GetDeviceUid(), false);
  // xor it
  size_t const offset = hash.size() / 4;
  string xoredHash;
  for (size_t i = 0; i < offset; ++i)
    xoredHash.push_back(hash[i] ^ hash[i + offset] ^ hash[i + offset * 2] ^ hash[i + offset * 3]);
  // and use base64 encoding
  return base64::encode(xoredHash);
}

@implementation IPhoneDownload

- (string const &) Url
{
  return m_url;
}

- (void) Cancel
{
	if (m_connection)
  	[m_connection cancel];
  m_progressObserver.clear();
  m_finishObserver.clear();
}

- (void) dealloc
{
//  NSLog(@"~IPhoneDownload() for url: %s", m_url.c_str());
  if (m_connection)
  {
  	[m_connection cancel];
  	[m_connection release];
  }
  // Non-zero means that download is canceled
	if (m_file)
  {
  	fclose(m_file);
    if (!m_requestedFileName.empty())
   		remove((m_requestedFileName + DOWNLOADING_FILE_EXTENSION).c_str());
  }
	[super dealloc];
}

- (NSMutableURLRequest *) CreateRequest
{
  // Create the request.
	NSMutableURLRequest * request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString: [NSString stringWithUTF8String:m_url.c_str()]]
  		cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:TIMEOUT_IN_SECONDS];
  long long fileSize = ftello(m_file);
  if (fileSize > 0)
  {
  	NSLog(@"Resuming download for file %s from position %qi", m_requestedFileName.c_str(), fileSize);
		NSString * val = [[NSString alloc] initWithFormat: @"bytes=%qi-", fileSize];
		[request addValue:val forHTTPHeaderField:@"Range"];
		[val release];
  }

  // send unique id in HTTP user agent header
  static string const uid = GetUniqueHashedId();
  [request addValue:[NSString stringWithUTF8String: uid.c_str()] forHTTPHeaderField:@"User-Agent"];
  return request;
}

- (BOOL) StartDownloadWithUrl: (char const *)originalUrl andFile: (char const *)file
		andFinishFunc: (TDownloadFinishedFunction &)finishFunc andProgressFunc: (TDownloadProgressFunction &)progressFunc
    andUseResume: (BOOL)resume
{
	m_finishObserver = finishFunc;
  m_progressObserver = progressFunc;

  m_retryCounter = 0;

	// try to create file first
  std::string tmpFile = file;
  tmpFile += DOWNLOADING_FILE_EXTENSION;
  m_file = fopen(tmpFile.c_str(), resume ? "ab" : "wb");
  if (m_file == 0)
  {
  	NSLog(@"Error opening %s file for download: %s", tmpFile.c_str(), strerror(errno));
  	// notify observer about error and exit
    if (m_finishObserver)
    	m_finishObserver(originalUrl, EHttpDownloadCantCreateFile);
    return NO;
  }

	m_requestedFileName = file;
	m_url = originalUrl;

	// create the connection with the request and start loading the data
	m_connection = [[NSURLConnection alloc] initWithRequest:[self CreateRequest] delegate:self];

	if (m_connection == 0)
  {
		NSLog(@"Can't create connection for url %s", originalUrl);
		// notify observer about error and exit
    if (m_finishObserver)
    	m_finishObserver(originalUrl, EHttpDownloadNoConnectionAvailable);
    return NO;
	}

  return YES;
}

- (void) connection: (NSURLConnection *)connection didReceiveResponse: (NSURLResponse *)response
{
	// This method is called when the server has determined that it
	// has enough information to create the NSURLResponse.

	// check if this is OK (not a 404 or the like)
  if ([response respondsToSelector:@selector(statusCode)])
  {
  	NSInteger statusCode = [(NSHTTPURLResponse *)response statusCode];
    if (statusCode < 200 || statusCode > 299)
    {
    	NSLog(@"Received HTTP error code %d, canceling download", statusCode);
      // deleting file
      fclose(m_file);
      m_file = 0;
      remove((m_requestedFileName + DOWNLOADING_FILE_EXTENSION).c_str());
      // notify user
		  if (m_finishObserver)
      	m_finishObserver(m_url.c_str(), statusCode == 404 ? EHttpDownloadFileNotFound : EHttpDownloadFailed);
  		// and selfdestruct...
  		GetDownloadManager().CancelDownload(m_url.c_str());
			return;
    }
  }

  // enable network activity indicator in top system toolbar
  [UIApplication sharedApplication].networkActivityIndicatorVisible = YES;
  
  m_projectedFileSize = [response expectedContentLength];
  // if server doesn't support resume, make sure we're downloading file from scratch
	if (m_projectedFileSize < 0)
  {
  	fclose(m_file);
    m_file = fopen((m_requestedFileName + DOWNLOADING_FILE_EXTENSION).c_str(), "wb");
  }
  NSLog(@"Projected file size: %qi", m_projectedFileSize);
}

- (void) connection: (NSURLConnection *)connection didReceiveData: (NSData *)data
{
	// Append the new data
	fwrite([data bytes], 1, [data length], m_file);
  if (m_progressObserver)
	  m_progressObserver(m_url.c_str(), TDownloadProgress(ftello(m_file), m_projectedFileSize));
}

- (void) connection: (NSURLConnection *)connection didFailWithError: (NSError *)error
{
	// inform the user
  NSLog(@"Connection failed for url %s\n%@", m_url.c_str(), [error localizedDescription]);

  // retry connection if it's network-specific error
  if ([error code] < 0 && ++m_retryCounter <= MAX_AUTOMATIC_RETRIES)
  {
    [m_connection release];
  	// create the connection with the request and start loading the data
		m_connection = [[NSURLConnection alloc] initWithRequest:[self CreateRequest] delegate:self];

		if (m_connection)
    {
    	NSLog(@"Retrying %d time", m_retryCounter);
    	return;	// successfully restarted connection
    }

    NSLog(@"Can't retry connection");
    // notify observer about error and exit after this if-block
  }

  if (m_finishObserver)
	  m_finishObserver(m_url.c_str(), EHttpDownloadFailed);
  // and selfdestruct...
  GetDownloadManager().CancelDownload(m_url.c_str());
}

- (void) connectionDidFinishLoading: (NSURLConnection *)connection
{
	// close file
  fclose(m_file);
  m_file = 0;
  // remote temporary extension from downloaded file
  remove(m_requestedFileName.c_str());
  bool resultForGUI = true;
  if (rename((m_requestedFileName + DOWNLOADING_FILE_EXTENSION).c_str(), m_requestedFileName.c_str()))
  {
  	resultForGUI = false;
  	NSLog(@"Can't rename to file %s", m_requestedFileName.c_str());
  }
  else
  {
  	NSLog(@"Successfully downloaded %s", m_url.c_str());
  }

  if (m_finishObserver)
	  m_finishObserver(m_url.c_str(), resultForGUI ? EHttpDownloadOk : EHttpDownloadFileIsLocked);
  // and selfdestruct...
  GetDownloadManager().CancelDownload(m_url.c_str());
}

@end
