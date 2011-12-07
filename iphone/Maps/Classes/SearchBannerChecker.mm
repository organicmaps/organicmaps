#import <UIKit/UIButton.h>
#import <UIKit/UIAlertView.h>
#import <UIKit/UIApplication.h>
#import "SearchBannerChecker.h"
#import "../../Common/GetActiveConnectionType.h"

#include "../../../defines.hpp"

#include "../../../platform/settings.hpp"
#include "../../../platform/http_request.hpp"

#include "../../../base/string_utils.cpp"

#include "../../../std/bind.hpp"
#include "../../../std/string.hpp"

#define SETTINGS_REDBUTTON_LAST_CHECK_TIME "RedbuttonLastCheck"
// at least 2 hours between next request to the server
#ifndef DEBUG
  #define MIN_SECONDS_ELAPSED_FOR_NEXT_CHECK 2*60*60
#else
  #define MIN_SECONDS_ELAPSED_FOR_NEXT_CHECK 10
#endif

// Return true if time passed from the last check is long enough
static bool ShouldCheckAgain()
{
  string strSecondsFrom1970;
  if (!Settings::Get(SETTINGS_REDBUTTON_LAST_CHECK_TIME, strSecondsFrom1970))
    return true;

  uint64_t secondsLastCheck;
  if (!strings::to_uint64(strSecondsFrom1970, secondsLastCheck))
    return true;

  uint64_t const secondsNow = (uint64_t)[[NSDate date] timeIntervalSince1970];

  if (secondsNow - secondsLastCheck > MIN_SECONDS_ELAPSED_FOR_NEXT_CHECK)
    return true;

  return false;
}

@implementation SearchBannerChecker

-(void) enableSearchButton:(id)searchButton andDownloadButton:(id)downloadButton
{
  UIButton * sb = (UIButton *)searchButton;
  UIButton * db = (UIButton *)downloadButton;
  if (sb.hidden == YES)
    {
    // Show Search button and swap them
    CGRect const sbFrame = sb.frame;
    CGRect const dbFrame = db.frame;
    sb.frame = dbFrame;
    db.frame = sbFrame;
    sb.hidden = NO;
    }
}

// Banner dialog handler
- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
  if (buttonIndex != alertView.cancelButtonIndex)
  {
    // Launch appstore
    string bannerUrl;
    Settings::Get(SETTINGS_REDBUTTON_URL_KEY, bannerUrl);
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:bannerUrl.c_str()]]];
  }
}

-(void) onRedbuttonServerReply:(downloader::HttpRequest &) r
{
  if (r.Status() == downloader::HttpRequest::ECompleted
      && (r.Data().find("itms") == 0 || r.Data().find("http") == 0))
  {
    // Redbutton is activated!!!
    // 1. Always enable search button
    // 2. Search button click displays banner
    // 3. Stop all future requests
    Settings::Set(SETTINGS_REDBUTTON_URL_KEY, r.Data());
    // Display banner
    UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"A paid version of MapsWithMe, featuring search, is available for download. Would you like to get it now?", @"Paid version has become available one-time dialog title in the free version")
                                                     message:nil
                                                    delegate:self
                                           cancelButtonTitle:NSLocalizedString(@"Cancel", @"Paid version has become available one-time dialog Negative button in the free version")
                                           otherButtonTitles:NSLocalizedString(@"Get it now", @"Paid version has become available one-time dialog Positive button in the free version"), nil];
    [alert show];
    [alert release];
    // Display search button
    [self enableSearchButton:m_searchButton andDownloadButton:m_downloadButton];
  }

  delete &r;
  // Save timestamp of the last check
  uint64_t const secondsNow = (uint64_t)[[NSDate date] timeIntervalSince1970];
  Settings::Set(SETTINGS_REDBUTTON_LAST_CHECK_TIME, strings::to_string(secondsNow));

  [m_searchButton release];
  [m_downloadButton release];
}

-(void) checkForBannerAndFixSearchButton:(id)searchButton
                       andDownloadButton:(id)downloadButton
{
  // Avoid all checks if we already enabled search button
  if (((UIButton *)searchButton).hidden == NO)
    return;

  // Check if redbutton was alredy activated and we should display the button
  string bannerUrl;
  if (Settings::Get(SETTINGS_REDBUTTON_URL_KEY, bannerUrl)
      && (bannerUrl.find("itms") == 0 || bannerUrl.find("http") == 0))
  {
    // Redbutton is activated. Enable Search button in the UI
    [self enableSearchButton:searchButton andDownloadButton:downloadButton];
  }
  else // Redbutton still is not activated.
  {
    // Check last timestamp to avoid often checks
    // and check if WiFi connection is active
    if (ShouldCheckAgain() && GetActiveConnectionType() == EConnectedByWiFi)
    {
      // Save buttons until we get server reply
      m_searchButton = [searchButton retain];
      m_downloadButton = [downloadButton retain];

      // Send request to the server
      // tricky boost::bind for objC class methods
		  typedef void (*OnResultFuncT)(id, SEL, downloader::HttpRequest &);
		  SEL onResultSel = @selector(onRedbuttonServerReply:);
		  OnResultFuncT onResultImpl = (OnResultFuncT)[self methodForSelector:onResultSel];
      downloader::HttpRequest::Get(REDBUTTON_SERVER_URL, bind(onResultImpl, self, onResultSel, _1));
    }
  }
}

@end
