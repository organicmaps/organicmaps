#import "IPhoneDownload.h"

#import <UIKit/UIApplication.h>

#include "../../platform/download_manager.hpp"
#include "../../platform/platform.hpp"

#define TIMEOUT_IN_SECONDS 15.0

static bool NeedToGenerateUrl(string const & url)
{
  return url.find("http://") != 0 && url.find("https://") != 0;
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
	[NSMutableURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithUTF8String:m_currentUrl.c_str()]]
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
	if (m_currentUrl.find("mapswithme.com") != string::npos)
	{
		static string const uid = GetPlatform().UniqueClientId();
		[request addValue:[NSString stringWithUTF8String: uid.c_str()] forHTTPHeaderField:@"User-Agent"];
	}
	return request;
}

- (BOOL) StartDownload: (HttpStartParams const &)params
{
  m_params = params;

  if (!params.m_fileToSave.empty())
  {
    // try to create file first
    string const tmpFile = m_params.m_fileToSave + DOWNLOADING_FILE_EXTENSION;
    m_file = fopen(tmpFile.c_str(), "ab");
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

  if (NeedToGenerateUrl(m_params.m_url))
    m_currentUrl = m_urlGenerator.PopNextUrl() + m_params.m_url;

	// create the connection with the request and start loading the data
	m_connection = [[NSURLConnection alloc] initWithRequest:[self CreateRequest] delegate:self];

  if (m_connection == 0)
  {
    NSLog(@"Can't create connection for url %s", m_currentUrl.c_str());
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

- (BOOL) shouldRetry: (NSError *)error
{
  NSInteger const code = [error code];
  return (code < 0 || code == 401 || code == 403 || code == 404) && NeedToGenerateUrl(m_params.m_url);
}

- (void) connection: (NSURLConnection *)connection didFailWithError: (NSError *)error
{
  // inform the user
  NSLog(@"Connection failed for url %s\n%@", m_currentUrl.c_str(), [error localizedDescription]);

  // retry connection if it's network-specific error
  if ([self shouldRetry:error])
  {
    string const newUrl = m_urlGenerator.PopNextUrl();
    if (!newUrl.empty())
    {
      m_currentUrl = newUrl + m_params.m_url;
      [m_connection release];
      // create the connection with the request and start loading the data
      m_connection = [[NSURLConnection alloc] initWithRequest:[self CreateRequest] delegate:self];

      if (m_connection)
        return;	// successfully restarted connection

      NSLog(@"Can't retry connection");
      // notify observer about error and exit after this if-block
    }
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
