//
//  main.m
//  Sloynik
//
//  Created by Yury Melnichek on 07.09.09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "PerfCount.h"

int main(int argc, char *argv[])
{
	LogTimeCounter("StartTime", "main");    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    int retVal = UIApplicationMain(argc, argv, nil, nil);
    [pool release];
    return retVal;
}
