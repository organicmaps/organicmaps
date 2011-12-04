#import <UIKit/UIButton.h>
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

-(void) onRedbuttonServerReply:(downloader::HttpRequest &) r
{
  if (r.Status() == downloader::HttpRequest::ECompleted
      && r.Data().find("http") == 0)
  {
    // Redbutton is activated!!!
    // 1. Always enable search button
    // 2. Search button click displays banner
    // 3. Stop all future requests
    Settings::Set(SETTINGS_REDBUTTON_URL_KEY, r.Data());
    UIButton * searchButton = (UIButton *)m_searchButton;
    if (searchButton.hidden == YES)
    {
      searchButton.hidden = NO;
      // Display banner
    }
  }

  delete &r;
  // Save timestamp of the last check
  uint64_t const secondsNow = (uint64_t)[[NSDate date] timeIntervalSince1970];
  Settings::Set(SETTINGS_REDBUTTON_LAST_CHECK_TIME, strings::to_string(secondsNow));
}

-(void) checkForBannerAndEnableButton:(id)searchButton
{
  m_searchButton = searchButton;

  // Check if we alredy should display the button
  string bannerUrl;
  if (Settings::Get(SETTINGS_REDBUTTON_URL_KEY, bannerUrl)
      && bannerUrl.find("http") == 0)
  {
    // Redbutton is activated. Enable Search button in the UI
    UIButton * button = (UIButton *)searchButton;
    button.hidden = NO;
  }
  else // Redbutton still is not activated.
  {
    // Check last timestamp to avoid often checks
    // and check if WiFi connection is active
    if (ShouldCheckAgain() && GetActiveConnectionType() == EConnectedByWiFi)
    {
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
