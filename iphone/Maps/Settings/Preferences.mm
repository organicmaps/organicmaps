#import "Preferences.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIAlertView.h>

#include "../../platform/settings.hpp"

@interface PrefDelegate : NSObject
{
}
@end
@implementation PrefDelegate
- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  // User decided to override Imperial system with metric one
  if (buttonIndex != 0)
    Settings::Set("Units", Settings::Metric);

  [self autorelease];
}
@end

@implementation Preferences

+ (void)setup
{
  Settings::Units u;
	if (!Settings::Get("Units", u))
  {
    // get system locale preferences
    BOOL const isMetric = [[[NSLocale autoupdatingCurrentLocale] objectForKey:NSLocaleUsesMetricSystem] boolValue];
    if (isMetric)
      u = Settings::Metric;
    else
    {
      u = Settings::Foot;
      PrefDelegate * d = [[PrefDelegate alloc] init];
      UIAlertView * alert = [[UIAlertView alloc] initWithTitle:@"Which measurement system do you prefer?"
                                                   message:nil
                                                  delegate:d cancelButtonTitle:nil 
                                         otherButtonTitles:@"US (ft/mi)", @"Metric (m/km)", nil];
      [alert show];
      [alert release];
    }
    Settings::Set("Units", u);
  }
}

@end
