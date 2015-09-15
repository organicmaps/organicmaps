#import "MapsAppDelegate.h"
#import "MWMConsole.h"

#include "Framework.h"

extern NSString * const kMwmTextToSpeechEnable;
extern NSString * const kMwmTextToSpeechDisable;

@implementation MWMConsole

+ (BOOL)performCommand:(NSString *)cmd
{
  if ([self performMapStyle:cmd])
    return YES;

  if ([self performSound:cmd])
    return YES;

  return NO;
}

+ (BOOL)performMapStyle:(NSString *)cmd
{
  // Hook for shell command on change map style
  BOOL const isDark = [cmd isEqualToString:@"mapstyle:dark"] || [cmd isEqualToString:@"?dark"];
  BOOL const isLight = isDark ? NO : [cmd isEqualToString:@"mapstyle:light"] || [cmd isEqualToString:@"?light"];
  BOOL const isOld = isLight || isDark ? NO : [cmd isEqualToString:@"?oldstyle"];

  if (!isDark && !isLight && !isOld)
    return NO;

  MapStyle const mapStyle = isDark ? MapStyleDark : (isOld ? MapStyleLight : MapStyleClear);
  [[MapsAppDelegate theApp] setMapStyle: mapStyle];

  return YES;
}

+ (BOOL)performSound:(NSString *)cmd
{
  // Hook for shell command on change map style
  BOOL const sound = [cmd isEqualToString:@"?sound"];
  BOOL const nosound = sound ? NO : [cmd isEqualToString:@"?nosound"];

  if (!sound && !nosound)
    return NO;

  // turn notification
  if (sound)
    [[NSNotificationCenter defaultCenter] postNotificationName:kMwmTextToSpeechEnable object:nil];
  if (nosound)
    [[NSNotificationCenter defaultCenter] postNotificationName:kMwmTextToSpeechDisable object:nil];

  return YES;
}

@end
