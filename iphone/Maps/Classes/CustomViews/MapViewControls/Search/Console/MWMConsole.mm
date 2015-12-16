#import "MapsAppDelegate.h"
#import "MWMConsole.h"

#include "Framework.h"

@implementation MWMConsole

+ (BOOL)performCommand:(NSString *)cmd
{
  if ([self performMapStyle:cmd])
    return YES;
  
  if ([self perform3dMode:cmd])
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

+ (BOOL)perform3dMode:(NSString *)cmd
{
  // Hook for shell command on change 3d mode
  BOOL const is3d = [cmd isEqualToString:@"?3d"];
  BOOL const is3dBuildings = [cmd isEqualToString:@"?b3d"];
  BOOL const is2d = [cmd isEqualToString:@"?2d"];
  
  if (!is3d && !is3dBuildings && !is2d)
    return NO;
  
  Framework & frm = GetFramework();
  frm.Save3dMode(is3d || is3dBuildings, is3dBuildings);
  frm.Allow3dMode(is3d || is3dBuildings, is3dBuildings);

  return YES;
}

@end
