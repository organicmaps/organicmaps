#import "Preferences.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIAlertView.h>
#import "../Classes/MapViewController.h"
#import "Framework.h"
#import "Statistics.h"

#include "../../platform/settings.hpp"

//********************* Helper delegate to handle async dialog message ******************
@interface PrefDelegate : NSObject
@property (nonatomic, weak) id m_controller;
@end

@implementation PrefDelegate

@end
//***************************************************************************************

@implementation Preferences

// TODO: Export this logic to C++

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
      [controller setupMeasurementSystem];
    }      
    else
    {
      u = Settings::Foot;
      // Will be released in delegate's callback itself
      PrefDelegate * d = [[PrefDelegate alloc] init];
      d.m_controller = controller;
    }

    /*This code counts conversion Guides->MWM. We rely on setting with name "Units", and by the time this code will be executed, Framework->GuidesManager should be initialised*/
    set<string> s;
    GetFramework().GetGuidesManager().GetGuidesId(s);
    NSMutableDictionary * guidesUrls = [[NSMutableDictionary alloc] init];
    for (set<string>::iterator it = s.begin(); it != s.end();++it)
    {
      NSString * countryUrl = [NSString stringWithUTF8String:it->c_str()];
      if ([[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:countryUrl]])
        guidesUrls[countryUrl] = @1;
    }
    if ([guidesUrls count])
      [[Statistics instance] logEvent:@"Guides Downloaded before MWM" withParameters:guidesUrls];

    Settings::Set("Units", u);
  }
  else
    [controller setupMeasurementSystem];
}

@end
