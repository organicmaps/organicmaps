#import "Preferences.h"
#import <Foundation/Foundation.h>

#include "../../platform/settings.hpp"

NSString * kMeasurementKey = @"measurementKey";

@implementation Preferences

- (id)init
{
  self = [super init];
  if (self)
  {
    [Preferences setupByPreferences];
    // listen for changes to our preferences when the Settings app does so,
    // when we are resumed from the backround, this will give us a chance to update our UI
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(defaultsChanged:)
                                                 name:NSUserDefaultsDidChangeNotification
                                               object:nil];
  }
  return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self
                                                  name:NSUserDefaultsDidChangeNotification
                                                object:nil];
  [super dealloc];
}

+ (void)setupByPreferences
{
  NSInteger measurement = [[NSUserDefaults standardUserDefaults] integerForKey:kMeasurementKey];
	if (measurement == 0)
  {
    // no default values have been set, create them here based on what's in our Settings bundle info
		NSString * pathStr = [[NSBundle mainBundle] bundlePath];
		NSString * settingsBundlePath = [pathStr stringByAppendingPathComponent:@"Settings.bundle"];
		NSString * finalPath = [settingsBundlePath stringByAppendingPathComponent:@"Root.plist"];

		NSDictionary * settingsDict = [NSDictionary dictionaryWithContentsOfFile:finalPath];
		NSArray * prefSpecifierArray = [settingsDict objectForKey:@"PreferenceSpecifiers"];

    
		NSNumber * measurementDefault = nil;

		NSDictionary * prefItem;
		for (prefItem in prefSpecifierArray)
    {
			NSString * keyValueStr = [prefItem objectForKey:@"Key"];
			id defaultValue = [prefItem objectForKey:@"DefaultValue"];
			
			if ([keyValueStr isEqualToString:kMeasurementKey])
      {
				measurementDefault = defaultValue;
        measurement = [measurementDefault integerValue];
      }
    }
  
    // since no default values have been set (i.e. no preferences file created), create it here		
		NSDictionary * appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 measurementDefault, kMeasurementKey,
                                 nil];

		[[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
		[[NSUserDefaults standardUserDefaults] synchronize];
  }

  // Update our settings because they're used inside framework сщку
  switch (measurement)
  {
  case 1: Settings::Set("Units", Settings::Metric); break;
  case 2: Settings::Set("Units", Settings::Yard); break;
  case 3: Settings::Set("Units", Settings::Foot); break;
  default: NSLog(@"Warning: Invalid measurement in preferences: %d", measurement);
  }
}

// we are being notified that our preferences have changed (user changed them in the Settings app)
// so read in the changes and update our UI.
- (void)defaultsChanged:(NSNotification *)notif
{
  [Preferences setupByPreferences];
}

@end
