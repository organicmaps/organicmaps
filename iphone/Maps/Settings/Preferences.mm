#import "Preferences.h"
#import "../Classes/MapViewController.h"

#include "Framework.h"

#include "platform/settings.hpp"

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
      u = Settings::Metric;
    else
    {
      u = Settings::Foot;
      // Will be released in delegate's callback itself
      PrefDelegate * d = [[PrefDelegate alloc] init];
      d.m_controller = controller;
    }

    Settings::Set("Units", u);
  }
}

@end
