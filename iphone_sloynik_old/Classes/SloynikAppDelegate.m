//
//  SloynikAppDelegate.m
//  Sloynik
//
//  Created by Yury Melnichek on 07.09.09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import "SloynikAppDelegate.h"
#import "RootViewController.h"
#import "PerfCount.h"


@implementation SloynikAppDelegate

@synthesize window;
@synthesize navigationController;


#pragma mark -
#pragma mark Application lifecycle

- (void)applicationDidFinishLaunching:(UIApplication *)application
{    
	LogTimeCounter("StartTime", "applicationDidFinishLaunching_begin");
	[window addSubview:[navigationController view]];
    [window makeKeyAndVisible];
	LogTimeCounter("StartTime", "applicationDidFinishLaunching_end");
}


- (void)applicationWillTerminate:(UIApplication *)application
{
	// Save data if appropriate
}


#pragma mark -
#pragma mark Memory management

- (void)dealloc
{
	[navigationController release];
	[window release];
	[super dealloc];
}


@end

