#import "MapsAppDelegate.h"
#import "MWMConsole.h"

#include "Framework.h"

@implementation MWMConsole

+ (BOOL)performCommand:(NSString *)cmd
{
  if ([self performMapStyle:cmd])
    return YES;

  return NO;
}

+ (BOOL)performMapStyle:(NSString *)cmd
{
  // Hook for shell command on change map style
  BOOL const isOld = [cmd isEqualToString:@"?oldstyle"];

  if (!isOld)
    return NO;

  MapStyle const mapStyle = MapStyleLight;
  [[MapsAppDelegate theApp] setMapStyle:mapStyle];
  return YES;
}

@end
