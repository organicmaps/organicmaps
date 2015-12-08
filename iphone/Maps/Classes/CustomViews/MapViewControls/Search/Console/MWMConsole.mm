#import "MapsAppDelegate.h"
#import "MWMConsole.h"

#include "Framework.h"

@implementation MWMConsole

+ (BOOL)performCommand:(NSString *)cmd
{
  if ([self performMapStyle:cmd])
    return YES;
  if ([self performRenderDebug:cmd])
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

// <MAPS.ME.Designer>
// Hook for shell command to turn on/off renderer debug mode
#import "drape/debug_rect_renderer.hpp"
+ (BOOL)performRenderDebug:(NSString *)cmd
{
  BOOL const isDebug = [cmd isEqualToString:@"?debug"];
  
  if (!isDebug)
    return NO;
  
  dp::DebugRectRenderer::Instance().SetEnabled(!dp::DebugRectRenderer::Instance().IsEnabled());
  return YES;
}
// </MAPS.ME.Designer>

@end
