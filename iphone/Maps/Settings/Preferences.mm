#import "Preferences.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIAlertView.h>
#import "../Classes/MapViewController.h"

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
      PrefDelegate * d = [[PrefDelegate alloc] init];
      d.m_controller = controller;
      UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Which measurement system do you prefer?", @"Choose measurement on first launch alert - title")
                                                   message:nil
                                                  delegate:d cancelButtonTitle:nil 
                                         otherButtonTitles:NSLocalizedString(@"Miles", @"Choose measurement on first launch alert - choose imperial system button"),
                                         NSLocalizedString(@"Kilometres", @"Choose measurement on first launch alert - choose metric system button"), nil];
      [alert show];
      [alert release];
    }
    Settings::Set("Units", u);
  }
  else
    [controller SetupMeasurementSystem];
}

@end
