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
  return m_params.m_url;
}

- (void) Cancel
{
	if (m_connection)
  	[m_connection cancel];
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
    if (!m_params.m_fileToSave.empty())
   		remove((m_params.m_fileToSave + DOWNLOADING_FILE_EXTENSION).c_str());
  }
	[super dealloc];
}

- (NSMutableURLRequest *) CreateRequest
{
  // Create the request.
	NSMutableURLRequest * request =
  [NSMutableURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithUTF8String:m_params.m_url.c_str()]]
  		cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:TIMEOUT_IN_SECONDS];
  if (m_file)
  {
    long long fileSize = ftello(m_file);
    if (fileSize > 0)
    {
      NSLog(@"Resuming download for file %s from position %qi",
            (m_params.m_fileToSave + DOWNLOADING_FILE_EXTENSION).c_str(),
            fileSize);
      NSString * val = [[NSString alloc] initWithFormat: @"bytes=%qi-", fileSize];
      [request addValue:val forHTTPHeaderField:@"Range"];
      [val release];
    }
  }
  if (!m_params.m_contentType.empty())
  {
    [request addValue:[NSString stringWithUTF8String: m_params.m_contentType.c_str()] forHTTPHeaderField:@"Content-Type"];
  }
  if (!m_params.m_postData.empty())
  {
    NSData * postData = [NSData dataWithBytes:m_params.m_postData.data() length:m_params.m_postData.size()];
    [request setHTTPBody:postData];
  }
  // set user-agent with unique client id only for mapswithme requests
  if (m_params.m_url.find("mapswithme.com") != string::npos)
  {
    static string const uid = GetUniqueHashedId();
    [request addValue:[NSString stringWithUTF8String: uid.c_str()] forHTTPHeaderField:@"User-Agent"];
  }
  return request;
}

- (BOOL) StartDownload: (HttpStartParams const &)params
{
  m_params = params;

  m_retryCounter = 0;

  if (!params.m_fileToSave.empty())
  {
    // try to create file first
    string const tmpFile = m_params.m_fileToSave + DOWNLOADING_FILE_EXTENSION;
    m_file = fopen(tmpFile.c_str(), params.m_useResume ? "ab" : "wb");
    if (m_file == 0)
    {
      NSLog(@"Error opening %s file for download: %s", tmpFile.c_str(), strerror(errno));
      // notify observer about error and exit
      if (m_params.m_finish)
      {
        HttpFinishedParams result;
        result.m_url = m_params.m_url;
        result.m_file = m_params.m_fileToSave;
        result.m_error = EHttpDownloadCantCreateFile;
        m_params.m_finish(result);
      }
      return NO;
    }
  }

	// create the connection with the request and start loading the data
	m_connection = [[NSURLConnection alloc] initWithRequest:[self CreateRequest] delegate:self];

	if (m_connection == 0)
  {
		NSLog(@"Can't create connection for url %s", params.m_url.c_str());
		// notify observer about error and exit
    if (m_params.m_finish)
    {
      HttpFinishedParams result;
      result.m_url = m_params.m_url;
      result.m_file = m_params.m_fileToSave;
      result.m_error = EHttpDownloadNoConnectionAvailable;
      m_params.m_finish(result);
    }
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
      if (m_file)
      {
        long long fileSize = ftello(m_file);
        fclose(m_file);
        m_file = 0;
        // delete file only if it's size is zero to resume download later
        if (fileSize == 0)
          remove((m_params.m_fileToSave + DOWNLOADING_FILE_EXTENSION).c_str());
      }
      // notify user
		  if (m_params.m_finish)
      {
        HttpFinishedParams result;
        result.m_url = m_params.m_url;
        result.m_file = m_params.m_fileToSave;
        result.m_error = statusCode == 404 ? EHttpDownloadFileNotFound : EHttpDownloadFailed;
        m_params.m_finish(result);
      }
  		// and selfdestruct...
  		GetDownloadManager().CancelDownload(m_params.m_url);
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
    m_file = fopen((m_params.m_fileToSave + DOWNLOADING_FILE_EXTENSION).c_str(), "wb");
  }
  NSLog(@"Projected file size: %qi", m_projectedFileSize);
}

- (void) connection: (NSURLConnection *)connection didReceiveData: (NSData *)data
{
	// Append the new data
  int64_t size = -1;
  if (m_file)
  {
    fwrite([data bytes], 1, [data length], m_file);
    size = ftello(m_file);
  }
  else
  {
    m_receivedBuffer.append(static_cast<char const *>([data bytes]), [data length]);
    size = m_receivedBuffer.size();
  }
  if (m_params.m_progress)
  {
    HttpProgressT progress;
    progress.m_url = m_params.m_url;
    progress.m_current = size;
    progress.m_total = m_projectedFileSize;
	  m_params.m_progress(progress);
  }
}

- (void) connection: (NSURLConnection *)connection didFailWithError: (NSError *)error
{
	// inform the user
  NSLog(@"Connection failed for url %s\n%@", m_params.m_url.c_str(), [error localizedDescription]);

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

  if (m_file)
  {
    long long fileSize = ftello(m_file);
    fclose(m_file);
    m_file = 0;
    // delete file only if it's size is zero to resume download later
    if (fileSize == 0)
      remove((m_params.m_fileToSave + DOWNLOADING_FILE_EXTENSION).c_str());
  }

  if (m_params.m_finish)
  {
    HttpFinishedParams result;
    result.m_url = m_params.m_url;
    result.m_file = m_params.m_fileToSave;
    result.m_error = EHttpDownloadFailed;
	  m_params.m_finish(result);
  }

  // and selfdestruct...
  GetDownloadManager().CancelDownload(m_params.m_url);
}

- (void) connectionDidFinishLoading: (NSURLConnection *)connection
{
  bool isLocked = false;
  if (m_file)
  {
    // close file
    fclose(m_file);
    m_file = 0;
    // remove temporary extension from downloaded file
    remove(m_params.m_fileToSave.c_str());
    if (rename((m_params.m_fileToSave + DOWNLOADING_FILE_EXTENSION).c_str(), m_params.m_fileToSave.c_str()))
    {
      isLocked = true;
      NSLog(@"Can't rename to file %s", m_params.m_fileToSave.c_str());
      // delete downloaded file
      remove((m_params.m_fileToSave + DOWNLOADING_FILE_EXTENSION).c_str());
    }
    else
    {
      NSLog(@"Successfully downloaded %s", m_params.m_url.c_str());
    }
  }
  if (m_params.m_finish)
  {
    HttpFinishedParams result;
    result.m_url = m_params.m_url;
    result.m_file = m_params.m_fileToSave;
    result.m_data.swap(m_receivedBuffer);
    result.m_error = isLocked ? EHttpDownloadFileIsLocked : EHttpDownloadOk;
	  m_params.m_finish(result);
  }
  // and selfdestruct...
  GetDownloadManager().CancelDownload(m_params.m_url);
}

@end
