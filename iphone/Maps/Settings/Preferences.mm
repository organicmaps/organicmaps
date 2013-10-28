#import "Preferences.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIAlertView.h>
#import "../Classes/MapViewController.h"
#import "Framework.h"
#import "Statistics.h"

#include "../../platform/settings.hpp"

//********************* Helper delegate to handle async dialog message ******************
@interface PrefDelegate : NSObject
@property (nonatomic, assign) id m_controller;
@end

@implementation PrefDelegate
@synthesize m_controller;
- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  // User decided to override Imperial system with metric one
  if (buttonIndex != 0)
    Settings::Set("Units", Settings::Metric);

  [m_controller SetupMeasurementSystem];

  [self autorelease];
}
@end
//***************************************************************************************

@implementation Preferences

+ (void)setup:(id)controller
{
  Settings::Units u;
	if (!Settings::Get("Units", u))
  {
    // get system locale preferences
    BOOL const isMetric = [[[NSLocale autoupdatingCurrentLocale] objectForKey:NSLocaleUsesMetricSystem] boolValue];
    if (isMetric)
    {
      u = Settings::Metric;
      [controller SetupMeasurementSystem];
    }      
    else
    {
      u = Settings::Foot;
      // Will be released in delegate's callback itself
      PrefDelegate * d = [[PrefDelegate alloc] init];
      d.m_controller = controller;
      UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"which_measurement_system", @"Choose measurement on first launch alert - title")
                                                   message:nil
                                                  delegate:d cancelButtonTitle:nil 
                                         otherButtonTitles:NSLocalizedString(@"miles", @"Choose measurement on first launch alert - choose imperial system button"),
                                         NSLocalizedString(@"kilometres", @"Choose measurement on first launch alert - choose metric system button"), nil];
      [alert show];
      [alert release];
    }

    /*This code counts conversion Guides->MWM. We rely on setting with name "Units", and by the time this code will be executed, Framework->GuidesManager should be initialised*/
    set<string> s;
    GetFramework().GetGuidesManager().GetGuidesId(s);
    NSMutableDictionary * guidesUrls = [[[NSMutableDictionary alloc] init] autorelease];
    for (set<string>::iterator it = s.begin(); it != s.end();++it)
    {
      NSString * countryUrl = [NSString stringWithUTF8String:it->c_str()];
      if ([APP canOpenURL:[NSURL URLWithString:countryUrl]])
        guidesUrls[countryUrl] = @1;
    }
    if ([guidesUrls count])
      [[Statistics instance] logEvent:@"Guides Downloaded before MWM" withParameters:guidesUrls];

    Settings::Set("Units", u);
  }
  else
    [controller SetupMeasurementSystem];
}

@end
