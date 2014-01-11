#import "MWMApi.h"
#import "Framework.h"
#import "MapsAppDelegate.h"

#include "../../../search/result.hpp"

@implementation MWMApi

+ (NSURL *)getBackUrl:(url_scheme::ApiPoint const &)apiPoint
{
  string const str = GetFramework().GenerateApiBackUrl(apiPoint);
  return [NSURL URLWithString:[NSString stringWithUTF8String:str.c_str()]];
}

+ (void)openAppWithPoint:(url_scheme::ApiPoint const &)apiPoint
{
  NSString * z = [NSString stringWithUTF8String:apiPoint.m_id.c_str()];
  NSURL * url = [NSURL URLWithString:z];
  if ([[UIApplication sharedApplication] canOpenURL:url])
    [[UIApplication sharedApplication] openURL:url];
  else
    [[UIApplication sharedApplication] openURL:[MWMApi getBackUrl:apiPoint]];
}

+ (BOOL)canOpenApiUrl:(url_scheme::ApiPoint const &)apiPoint
{
  NSString * z = [NSString stringWithUTF8String:apiPoint.m_id.c_str()];
  if ([[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:z]])
    return YES;
  if ([[UIApplication sharedApplication] canOpenURL:[MWMApi getBackUrl:apiPoint]])
    return YES;
  return NO;
}

@end
