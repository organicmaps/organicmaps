/*******************************************************************************

 Copyright (c) 2013, MapsWithMe GmbH
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************/

#import "MapsWithMeAPI.h"

#define MAPSWITHME_API_VERSION 1

static NSString * MWMUrlScheme = @"mapswithme://";

@implementation MWMPin

- (id) init
{
  if ((self = [super init]))
  {
    self.lat = INFINITY;
    self.lon = INFINITY;
  }
  return self;
}

- (id) initWithLat:(double)lat lon:(double)lon title:(NSString *)title and:(NSString *)idOrUrl
{
  if ((self = [super init]))
  {
    self.lat = lat;
    self.lon = lon;
    self.title = title;
    self.idOrUrl = idOrUrl;
  }
  return self;
}

- (void)dealloc
{
  self.title = nil;
  self.idOrUrl = nil;
  [super dealloc];
}
@end

// Utility class to automatically handle "MapsWithMe is not installed" situations
@interface MWMNavigationController : UINavigationController
@end
@implementation MWMNavigationController
- (void)onCloseButtonClicked:(id)sender
{
  [self dismissModalViewControllerAnimated:YES];
}
@end


@implementation MWMApi

+ (BOOL) isMapsWithMeUrl:(NSURL *)url
{
  NSString * appScheme = [MWMApi detectBackUrlScheme];
  return appScheme && [url.scheme isEqualToString:appScheme];
}

+ (MWMPin *) pinFromUrl:(NSURL *)url
{
  if (![MWMApi isMapsWithMeUrl:url])
    return nil;

  MWMPin * pin = nil;
  if ([url.host isEqualToString:@"pin"])
  {
    pin = [[[MWMPin alloc] init] autorelease];
    for (NSString * param in [url.query componentsSeparatedByString:@"&"])
    {
      NSArray * values = [param componentsSeparatedByString:@"="];
      if (values.count == 2)
      {
        NSString * key = [values objectAtIndex:0];
        if ([key isEqualToString:@"ll"])
        {
          NSArray * coords = [param componentsSeparatedByString:@","];
          if (coords.count == 2)
          {
            pin.lat = [[NSDecimalNumber decimalNumberWithString:[coords objectAtIndex:0]] doubleValue];
            pin.lon = [[NSDecimalNumber decimalNumberWithString:[coords objectAtIndex:1]] doubleValue];
          }
        }
        else if ([key isEqualToString:@"n"])
          pin.title = [[values objectAtIndex:1] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
        else if ([key isEqualToString:@"id"])
          pin.idOrUrl = [[values objectAtIndex:1] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
        else
          NSLog(@"Unsupported url parameters: %@", values);
      }
    }
    // do not accept invalid coordinates
    if (pin.lat > 90. || pin.lat < -90. || pin.lon > 180. || pin.lon < -180.)
      pin = nil;
  }
  return pin;
}

+ (BOOL) isApiSupported
{
  return [[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:MWMUrlScheme]];
}

+ (BOOL) showMap
{
  return [[UIApplication sharedApplication] openURL:[NSURL URLWithString:[MWMUrlScheme stringByAppendingFormat:@"map?v=%d", MAPSWITHME_API_VERSION]]];
}

+ (BOOL) showLat:(double)lat lon:(double)lon title:(NSString *)title and:(NSString *)idOrUrl
{
  MWMPin * pin = [[[MWMPin alloc] initWithLat:lat lon:lon title:title and:idOrUrl] autorelease];
  return [MWMApi showPin:pin];
}

+ (BOOL) showPin:(MWMPin *)pin
{
  return [MWMApi showPins:[NSArray arrayWithObject:pin]];
}

+ (BOOL) showPins:(NSArray *)pins
{
  // Automatic check that MapsWithMe is installed
  if (![MWMApi isApiSupported])
  {
    // Display dialog with link to the app
    [MWMApi showMapsWithMeIsNotInstalledDialog];
    return NO;
  }

  NSString * appName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"];
  NSMutableString * str = [[NSMutableString alloc] initWithFormat:@"%@map?v=%d&appname=%@&", MWMUrlScheme, MAPSWITHME_API_VERSION,
                           [appName stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];

  NSString * backUrlScheme = [MWMApi detectBackUrlScheme];
  if (backUrlScheme)
    [str appendFormat:@"backurl=%@&", [backUrlScheme stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];

  for (MWMPin * point in pins)
  {
    [str appendFormat:@"ll=%f,%f&", point.lat, point.lon];
    @autoreleasepool
    {
      if (point.title)
        [str appendFormat:@"n=%@&", [point.title stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
      if (point.idOrUrl)
        [str appendFormat:@"id=%@&", [point.idOrUrl stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
    }
  }

  NSURL * url = [[NSURL alloc] initWithString:str];
  [str release];
  BOOL const result = [[UIApplication sharedApplication] openURL:url];
  [url release];
  return result;
}

+ (NSString *) detectBackUrlScheme
{
  for (NSDictionary * dict in [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleURLTypes"])
  {
    if ([[dict objectForKey:@"CFBundleURLName"] rangeOfString:@"mapswithme" options:NSCaseInsensitiveSearch].location != NSNotFound)
    {
      for (NSString * scheme in [dict objectForKey:@"CFBundleURLSchemes"])
      {
        if ([scheme rangeOfString:@"mapswithme" options:NSCaseInsensitiveSearch].location != NSNotFound)
          return scheme;
      }
    }
  }
  NSLog(@"WARNING: No com.mapswithme.maps url schemes are added in the Info.plist file. Please add them if you want API users to come back to your app.");
  return nil;
}

// HTML page for users who didn't install MapsWithMe
static NSString * mapsWithMeIsNotInstalledPage =
@"<html>" \
"<head>" \
"<title>Please install MapsWithMe - offline maps of the World</title>" \
"<meta name='viewport' content='width=device-width, initial-scale=1.0'/>" \
"<meta charset='UTF-8'/>" \
"<style type='text/css'>" \
"body { font-family: Roboto,Helvetica; background-color:#fafafa; text-align: center;}" \
".description { text-align: center; font-size: 0.85em; margin-bottom: 1em; }" \
".button { -moz-border-radius: 20px; -webkit-border-radius: 20px; -khtml-border-radius: 20px; border-radius: 20px; padding: 10px; text-decoration: none; display:inline-block; margin: 0.5em; }" \
".shadow { -moz-box-shadow: 3px 3px 5px 0 #444; -webkit-box-shadow: 3px 3px 5px 0 #444; box-shadow: 3px 3px 5px 0 #444; }" \
".lite { color: white; background-color: #333; }" \
".pro  { color: white; background-color: green; }" \
".mwm { color: green; text-decoration: none; }" \
"</style>" \
"</head>" \
"<body>" \
"<div class='description'><a href='http://mapswith.me' target='_blank' class='mwm'>MapsWithMe</a> app should be installed to view the map.</div>" \
"<a href='http://mapswith.me/app?api' class='lite button shadow'>Download&nbsp;MapsWithMe&nbsp;Lite&nbsp;(free)</a>" \
"<a href='http://mapswith.me/get?api' class='pro button shadow'>Download&nbsp;MapsWithMe&nbsp;Pro</a>" \
"</body>" \
"</html>";


// For gethostbyname below
#include <netdb.h>

+ (void) showMapsWithMeIsNotInstalledDialog
{
  UIWebView * webView = [[[UIWebView alloc] initWithFrame:[[UIScreen mainScreen] applicationFrame]] autorelease];
  // check that we have Internet connection and display fresh online page if possible
  if (gethostbyname("mapswith.me"))
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"http://mapswith.me/api_mwm_not_installed"]]];
  else
    [webView loadHTMLString:mapsWithMeIsNotInstalledPage baseURL:[NSURL URLWithString:@"http://mapswith.me/"]];
  UIViewController * webController = [[[UIViewController alloc] init] autorelease];
  webController.view = webView;
  webController.title = @"Install MapsWithMe";
  MWMNavigationController * navController = [[[MWMNavigationController alloc] initWithRootViewController:webController] autorelease];
  navController.navigationBar.topItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:@"Close" style:UIBarButtonItemStyleDone target:navController action:@selector(onCloseButtonClicked:)];

  [[[UIApplication sharedApplication] delegate].window.rootViewController presentModalViewController:navController animated:YES];
}

@end
